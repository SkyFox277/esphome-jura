# Contributing

Contributions are welcome — especially protocol discoveries, new machine support, and bug reports.

## Adding support for a new machine

1. Capture raw `IC:` and `RT:0000` logs from your machine (use the `logger:` component at `DEBUG` level)
2. Compare IC: byte 0 across different machine states (tray missing, tank empty, cleaning needed) to identify bit positions
3. Add a new profile to `MACHINE_PROFILES` in `components/jura_coffee/__init__.py`
4. Document the confirmed/unconfirmed bits in the profile comment (see existing profiles for the pattern)
5. Update the "Known working models" table in `README.md`

## Reporting a protocol discovery

Open an issue with:
- Machine model and firmware version (if known)
- Raw command/response pairs (`IC:`, `RT:`, `RR:`, `FA:`, etc.)
- What you were doing when you captured the data
- Your hypothesis about what the values mean

## Workflow

All contributions go through pull requests — **never push directly to `main`**.

1. Fork the repository
2. Create a branch: `feat/your-feature` or `fix/your-bug`
3. Commit your changes (Conventional Commits)
4. Open a pull request against `main`

## Pull request checklist

- One logical change per PR
- Follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages
- Code in English (variable names, comments)
- Test on real hardware before submitting if possible
- All sensors must be optional (no required YAML keys beyond `uart_id`)

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
