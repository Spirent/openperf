import client
from datetime import datetime
import logging
import os
import subprocess


class Service(object):
    def __init__(self, config):
        self.config = config

    def client(self):
        # Create an API client using the service's base URL
        cconfig = client.Configuration()
        cconfig.host = self.config.base_url
        return client.ApiClient(cconfig)

    def start(self):
        # Start the service as a subprocess
        with open("%s.log" % self.config.name, "w+") as log:
            p = subprocess.Popen(self.config.command, stdout=log, stderr=subprocess.STDOUT, shell=True)

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
