use strict;
use warnings;
use TestLib;
use Test::More tests => 1;

use FindBin;
use lib $FindBin::RealBin;

use RewindTest;

# Test that running pg_rewind if the two clusters are on the same
# timeline runs successfully.

RewindTest::setup_cluster();
RewindTest::start_master();
RewindTest::create_standby();
local $ENV{'SUSPEND_PG_REWIND'}="1";
command_like(
	RewindTest::run_pg_rewind('local'),
	qr/pg_rewind suspended/,
	'pg_rewind gets suspended');
RewindTest::clean_rewind_test();
exit(0);
