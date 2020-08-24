# -*- coding: utf-8 -*-

from mamba.reporter import Reporter as MambaReporter


class Reporter(MambaReporter):

    def start(self):
        super().start()
        self.skipped_count = 0

    def example_skipped(self, example):
        self.skipped_count += 1
        self.notify('example_skipped', example)

    def finish(self):
        self.stop()
        self.notify('summary',
                    self.duration,
                    self.example_count,
                    self.failed_count,
                    self.pending_count,
                    self.skipped_count)
        self.notify('failures', self.failed_examples)
