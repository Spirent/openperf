# -*- coding: utf-8 -*-

import sys
import traceback
import inspect
from datetime import datetime
from xml.etree import ElementTree

from clint.textui import indent, puts, colored
from mamba.formatters import (Formatter as MambaFormatter,
                              DocumentationFormatter as MambaDocumentationFormatter,
                              ProgressFormatter as MambaProgressFormatter,
                              JUnitFormatter as MambaJUnitFormatter)


class Formatter(MambaFormatter):

    def example_skipped(self, example):
        pass

    def summary(self, duration, example_count, failed_count, pending_count, skipped_count):
        pass


class DocumentationFormatter(MambaDocumentationFormatter):

    def example_skipped(self, example):
        self._format_example(self._color('yellow', '*'), example)

    def summary(self, duration, example_count, failed_count, pending_count, skipped_count):
        duration = self._format_duration(duration)
        if failed_count != 0:
            puts(self._color('red', "%d examples failed of %d ran in %s" %
                             (failed_count, example_count, duration)))
        elif pending_count + skipped_count != 0:
            puts(self._color('yellow', "%d examples ran (%d skipped, %d pending) in %s" %
                             (example_count, skipped_count, pending_count, duration)))
        else:
            puts(self._color('green', "%d examples ran in %s" %
                             (example_count, duration)))


class ProgressFormatter(MambaProgressFormatter, DocumentationFormatter):

    def example_skipped(self, example):
        puts(self._color('yellow', '*'), newline=False)

    def summary(self, duration, example_count, failed_count, pending_count, skipped_count):
        puts()
        puts()
        DocumentationFormatter.summary(self,
                                       duration, example_count, failed_count, pending_count, skipped_count)


class JUnitFormatter(MambaJUnitFormatter):
    def example_skipped(self, example):
        self._dump_example(example, ElementTree.Element('skipped'))

    def summary(self, duration, example_count, failed_count, pending_count, skipped_count):
        self.suite.attrib['tests'] = str(example_count)
        self.suite.attrib['skipped'] = str(pending_count + skipped_count)
        self.suite.attrib['failures'] = str(failed_count)
        self.suite.attrib['time'] = str(duration)
        ElementTree.ElementTree(self.suite).write(
            sys.stdout, encoding='unicode')
