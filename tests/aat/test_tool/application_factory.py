# -*- coding: utf-8 -*-

from mamba.application_factory import ApplicationFactory as MambaApplicationFactory
from mamba.runners import CodeCoverageRunner
from runners import BaseRunner
import loader
import formatters
import reporter


class ApplicationFactory(MambaApplicationFactory):

    def runner(self):
        runner = BaseRunner(self._example_collector(),
                            self._loader(),
                            self._reporter(),
                            self.settings.tags)

        if self.settings.enable_code_coverage:
            runner = \
                CodeCoverageRunner(runner,
                                   self.settings.code_coverage_file)

        return runner

    def _loader(self):
        return loader.Loader()

    def _reporter(self):
        return reporter.Reporter(self._formatter())

    def _formatter(self):
        if self.settings.format == 'progress':
            return formatters.ProgressFormatter(self.settings)
        if self.settings.format == 'documentation':
            return formatters.DocumentationFormatter(self.settings)
        if self.settings.format == 'junit':
            return formatters.JUnitFormatter(self.settings)

        return self._custom_formatter()
