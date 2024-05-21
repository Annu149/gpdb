package cli

import (
	"fmt"
	"github.com/greenplum-db/gp-common-go-libs/gplog"
	"github.com/greenplum-db/gpdb/gp/constants"
	"github.com/greenplum-db/gpdb/gp/hub"
	"github.com/spf13/cobra"
	"os/exec"
	"path/filepath"
)

func deleteCmd() *cobra.Command {
	startCmd := &cobra.Command{
		Use:     "delete",
		Short:   "de-register services and cleanup related files",
		PreRunE: InitializeCommand,
		RunE:    DeleteServices,
	}
	return startCmd
}

var (
	execCommand      = exec.Command
	DeleteServices   = DeleteServicesFunc
	RemoveConfigFile = RemoveConfigFileFunc
)

func DeleteServicesFunc(cmd *cobra.Command, args []string) error {
	Conf = &hub.Config{}
	err := Conf.Load(ConfigFilePath)
	if err != nil {
		return err
	}

	// stop services if running, else ignore the error
	err = RunStopServices(cmd, args)

	hostList := make([]string, 0)
	for _, host := range Conf.Hostnames {
		hostList = append(hostList, "-h", host)
	}

	//	remove gp.conf from hub and agents using ssh
	err = RemoveConfigFile(hostList)
	if err != nil {
		return err
	}

	//	launchctl/systemctl stop and unload
	//serviceDir = fmt.Sprintf(DefaultServiceDir, serviceUser)

	err = Platform.RemoveHubService(Conf.ServiceName, serviceDir)
	if err != nil {
		return err
	}

	err = Platform.RemoveAgentService(gpHome, Conf.ServiceName, serviceDir, hostList)
	if err != nil {
		return err
	}

	err = Platform.RemoveHubServiceFile(Conf.ServiceName, serviceDir)
	if err != nil {
		return err
	}

	err = Platform.RemoveAgentServiceFile(gpHome, Conf.ServiceName, serviceDir, hostList)
	if err != nil {
		return err
	}
	return nil
}

func RemoveConfigFileFunc(hostList []string) error {
	cmdArgs := append(hostList, "rm", ConfigFilePath)
	utility := filepath.Join(gpHome, "bin", constants.GpSSH)
	err := execCommand(utility, cmdArgs...).Run()
	if err != nil {
		return fmt.Errorf("could not delete %s on hosts: %w", ConfigFilePath, err)
	}
	gplog.Info("Successfully deleted gp.conf file")
	return nil
}
