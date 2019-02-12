import client
from datetime import datetime
from distutils.util import strtobool
import logging
import os
import subprocess
import shlex

class Service(object):
    def __init__(self, config):
        self.config = config

    def client(self):
        # Create an API client using the service's base URL
        cconfig = client.Configuration()
        cconfig.host = self.config.base_url
        cconfig.debug = strtobool(os.environ.get('MAMBA_DEBUG', 'False'))
        return client.ApiClient(cconfig)

    def start(self, **kwargs):
        # Check for stale files from a previous run.
        # NOTE: This will NOT verify if another copy of inception is running.
        files_to_clean = ['/dev/shm/com.spirent.inception.memory', '/tmp/.com.spirent.inception/server']
        for file_name in files_to_clean:
            if os.path.exists(file_name):
                os.remove(file_name)

        # Start the service as a subprocess
        with open("%s.log" % kwargs.get('log_file_name', self.config.name), "w+") as log:
            p = subprocess.Popen(shlex.split(self.config.command), stdout=log, stderr=subprocess.STDOUT)

        # Wait for the service to initialize
        try:
            self._wait()
        except:
            p.kill()
            p.wait()
            raise

        return p

    def _wait(self):
        # Create an API client using the service's init URL (falling back to
        # the base URL when there is no init URL set)
        cconfig = client.Configuration()
        try:
            cconfig.host = self.config.init_url
        except AttributeError:
            cconfig.host = self.config.base_url
        cconfig.logger["urllib3_logger"].setLevel(logging.ERROR)

        init_client = client.ApiClient(cconfig)

        # If there is an init timeout, then wait that long for the service to
        # initialize, otherwise expect it to initialize immediately
        deadline = datetime.now()
        try:
            deadline += self.config.init_timeout
        except AttributeError:
            pass

        while True:
            # Expect a 2xx response from the service
            try:
                resp = init_client.call_api("", "GET")
                if resp[1] / 100 == 2:
                    return
            except Exception as e:
                err = e
            else:
                err = None

            # Give up once the deadline is reached
            if datetime.now() > deadline:
                if err:
                    raise Exception('timed out waiting for %s service to initialize: last error was %s' % (self.config.name, err))
                else:
                    raise Exception('timed out waiting for %s service to initialize: last status from %s was %s' % (self.config.name, cconfig.host, resp[1]))

    def _check_capabilities(self, binary):
        required_capabilities = ['CAP_IPC_LOCK', 'CAP_NET_RAW', 'CAP_SYS_RAWIO', 'CAP_IPC_OWNER', 'CAP_NET_BIND_SERVICE', 'CAP_SETUID', 'CAP_SYS_ADMIN', 'CAP_FOWNER', 'CAP_DAC_OVERRIDE']

        cap_str = ','.join(required_capabilities) + "=epi"

        try:
            subprocess.check_output(['setcap', '-v', cap_str, binary])
        except subprocess.CalledProcessError:
            warning = "Warning: suggested capabilities not found on file " + binary + ". "
            warning += "Falling back on sudo method, which can leave test environment in an unclean state."
            print warning
            return False

        return True
