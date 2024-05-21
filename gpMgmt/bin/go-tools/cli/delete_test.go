package cli_test

import (
	"encoding/json"
	"fmt"
	"github.com/greenplum-db/gpdb/gp/agent"
	"github.com/greenplum-db/gpdb/gp/cli"
	"github.com/greenplum-db/gpdb/gp/hub"
	"github.com/greenplum-db/gpdb/gp/testutils"
	"github.com/greenplum-db/gpdb/gp/testutils/exectest"
	"github.com/greenplum-db/gpdb/gp/utils"
	"github.com/spf13/cobra"
	"os"
	"testing"
)

func TestDeleteServices(t *testing.T) {
	setupTest(t)
	defer teardownTest()

	platform := &testutils.MockPlatform{Err: nil}
	hubPlatform := &testutils.MockPlatform{Err: nil}
	agent.SetPlatform(platform)
	hub.SetPlatform(hubPlatform)
	defer agent.ResetPlatform()
	defer hub.ResetPlatform()

	cmd := cobra.Command{}

	file, err := os.CreateTemp("", "test")
	if err != nil {
		t.Fatalf("unexpected error: %#v", err)
	}
	defer os.Remove(file.Name())

	configHandle, err := os.OpenFile(file.Name(), os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
	if err != nil {
		t.Fatalf("could not create configuration file %s: %v\n", file.Name(), err)
	}
	defer configHandle.Close()
	configContents, err := json.MarshalIndent(cli.Conf, "", "\t")
	if err != nil {
		t.Fatalf("could not parse configuration file %s: %v\n", file.Name(), err)
	}

	_, err = configHandle.Write(configContents)
	if err != nil {
		t.Fatalf("could not write to configuration file %s: %v\n", file.Name(), err)
	}

	cli.ConfigFilePath = file.Name()

	t.Run("DeleteServices does not fail if RunStopServices fails", func(t *testing.T) {
		defer resetCLIVars()
		utils.RemoveServiceCommand = exectest.NewCommand(exectest.Success)
		utils.UnloadServiceCommand = exectest.NewCommand(exectest.Success)
		utils.LoadServiceCommand = exectest.NewCommand(exectest.Success)
		utils.SetExecCommand(exectest.NewCommand(exectest.Success))
		defer utils.ResetExecCommand()

		cli.RunStopServices = func(cmd *cobra.Command, args []string) error {
			return fmt.Errorf("error")
		}

		cli.RemoveConfigFile = func(hostList []string) error {
			return nil
		}

		err := cli.DeleteServices(&cmd, nil)
		if err != nil {
			t.Fatalf("unexpected error %#v", err)
		}
	})
	t.Run("DeleteServices fails when removal of conf file fails", func(t *testing.T) {
		defer resetCLIVars()

		cli.RunStopServices = func(cmd *cobra.Command, args []string) error {
			return nil
		}

		cli.RemoveConfigFile = func(hostList []string) error {
			return fmt.Errorf("error")
		}

		err := cli.DeleteServices(&cmd, nil)
		expectedErr := "error"
		if err.Error() != expectedErr {
			t.Fatalf("got %q, want %q", err, expectedErr)
		}
	})
}
