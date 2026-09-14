# GuardCAN AutoHack ML data contract

This is a research proof-of-concept preprocessing package, aligned with
industry-style traceability and leakage-control practices. It is not certified,
production-ready, or compliant with any automotive standard.

Raw AutoHack CSVs are supplied by configuration and are never copied into Git.
The implementation uses standard-library CSV streaming and processes each file
and interface as an isolated temporal stream.

Set the local path without committing it:

```powershell
$env:AEGIS_AUTOHACK_ROOT = 'C:\path\to\AutoHack\extracted'
```

Example:

```powershell
python -m unittest discover -s tests\ml -p "test_*.py"
```

The package does not train a model. `Label`, filenames, capture identity,
vehicle identity, split membership, and future-derived values are excluded from
the feature contract.
