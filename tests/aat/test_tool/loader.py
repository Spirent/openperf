# -*- coding: utf-8 -*-

from example_group import ExampleGroup
from example import Example
from mamba.example_group import PendingExampleGroup, SharedExampleGroup
from mamba.example import PendingExample
from mamba.loader import Loader as MambaLoader


class Loader(MambaLoader):
    def _create_example_group(self, klass):
        name = klass._example_name
        tags = klass._tags

        if klass._pending:
            return PendingExampleGroup(name, tags=tags)
        elif klass._shared:
            return SharedExampleGroup(name, tags=tags)
        return ExampleGroup(name, tags=tags)

    def _load_examples(self, klass, example_group):
        for example in self._examples_in(klass):
            tags = example._tags
            if self._is_pending_example(example) or self._is_pending_example_group(example_group):
                example_group.append(PendingExample(
                    example, tags=tags, module=self.module))
            else:
                example_group.append(
                    Example(example, tags=tags, module=self.module))
