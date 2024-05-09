#!/usr/bin/env python3
#
# Copyright (c) Greenplum Inc 2008. All Rights Reserved.
#
# Unit Testing of catalog module.
#

from mock import call, Mock, patch, ANY, MagicMock
from gppylib.test.unit.gp_unittest import GpTestCase, FakeCursor
from gppylib.util.gp_utils import get_startup_recovery_remaining_bytes
from gppylib.util.gp_utils import split_walfile_name


class GpUtilsTestCase(GpTestCase):
    def setUp(self):
        mock_logger = Mock(spec=['log', 'warn', 'info', 'debug', 'error', 'warning', 'fatal'])
        self.apply_patches([
            patch('gppylib.commands.gp.Command.__init__', return_value=None),
            patch('gppylib.commands.gp.Command.run', return_value=None),
            patch('gppylib.commands.gp.Command.get_return_code', return_value=0),
            patch('gppylib.commands.gp.Command.get_stdout', side_effect=["000000030000052D0000000C", "0000000100000BAD00000025"]),
            patch('gppylib.commands.pg.PgControlData.run'),
            patch('gppylib.commands.pg.PgControlData.get_value', return_value="BAD/9698CA28"),
            patch('gppylib.db.dbconn.DbURL'),
            patch('gppylib.db.dbconn.connect'),
            patch('gppylib.db.dbconn.querySingleton'),
            patch('gppylib.util.gp_utils.logger', return_value=mock_logger)
        ])

        self.mock_logger = self.get_mock_from_apply_patch('logger')

    @patch('gppylib.commands.gp.Command.get_return_code', return_value=1)
    def test_get_startup_recovery_remaining_bytes_command_fails(self, mock):
        self.assertEqual(get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1"), None)
        self.assertEqual([call("No startup recovery process running")], self.mock_logger.debug.call_args_list)

    @patch('gppylib.commands.gp.Command.get_stdout', return_value="")
    def test_get_startup_recovery_remaining_bytes_returns_None(self, mock):
        self.assertEqual(get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1"), None)
        self.assertEqual([call("Could not fetch startup recovery walfilename")], self.mock_logger.debug.call_args_list)

    @patch('gppylib.db.dbconn.connect', side_effect=Exception('Error'))
    def test_get_startup_recovery_remaining_bytes_db_connect_raises_exception(self, mock):
        get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1")
        self.assertEqual([call("Failed to get wal_segment_size, err: Error")], self.mock_logger.debug.call_args_list)

    @patch('gppylib.db.dbconn.querySingleton', side_effect=Exception('Error'))
    def test_get_startup_recovery_remaining_bytes_db_query_raises_exception(self, mock):
        get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1")
        self.assertEqual([call("Failed to get wal_segment_size, err: Error")], self.mock_logger.debug.call_args_list)

    @patch('gppylib.util.gp_utils.split_walfile_name', side_effect=Exception('Error'))
    def test_get_startup_recovery_remaining_bytes_splitwalfilename_fails(self, mock):
        get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1")
        self.assertEqual([call("Failed to split walfile_name, err: Error")],
                         self.mock_logger.debug.call_args_list)

    @patch('gppylib.commands.gp.Command.get_return_code', side_effect=[0, 1])
    def test_get_startup_recovery_pg_waldump_fails(self, mock):
        self.assertEqual(get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1"), None)
        self.assertEqual([call("Could not fetch last valid walfile name")], self.mock_logger.debug.call_args_list)

    @patch('gppylib.db.dbconn.querySingleton', side_effect=[67108864])
    def test_get_startup_recovery_remaining_bytes_succeeds(self, mock):
        self.assertEqual(get_startup_recovery_remaining_bytes("sdw1", 7002, "/data/demo/1"), 7148503302144)

    def test_split_walfile_name_raises_exception(self):
        with self.assertRaises(Exception) as ex:
            split_walfile_name("000000030000052D000000.C", 67108864)
        self.assertTrue('Invalid WAL file name "000000030000052D000000.C"' in str(ex.exception))

    def test_split_walfile_name_succeeds(self):
        tli, segno = split_walfile_name("000000030000052D0000000C", 67108864)
        self.assertEqual(tli, 3)
        self.assertEqual(segno, 84812)



