# Contributing to ZenOS

Thank you for contributing to ZenOS.

## Before submitting changes

- Keep implementation, configuration, tests, and documentation aligned with the actual source behavior.
- Do not introduce claims of certification, standards compliance, benchmark guarantees, or safety properties that are not demonstrated by the repository.
- Preserve the project's zero-heap/static-resource design and avoid unnecessary RAM growth.
- For scheduler changes, verify priority semantics, ready-queue behavior, tick behavior, and context-switch assumptions.
- For safety-related changes, document whether a mechanism is enabled by default and what it actually guarantees.

## Validation

Run the repository test suite locally:

```text
python -m unittest discover -s tests -p "test_*.py" -v
```

Documentation changes should also be checked against `docs/CURRENT_IMPLEMENTATION.md` and the release-specific page under `docs/guide-html/current.html`.

## Pull requests

Please keep pull requests focused and explain any behavior or API changes. Include the target configuration or hardware profile when a change depends on one.
