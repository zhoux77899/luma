# Contributing

## Commit messages

Use this format for every commit:

```text
{type}({scope}): {description}

{details}

Signed-off-by: {git.user.name} <{git.user.email}>
```

Keep commit messages in English. Use a concise, imperative description and explain
the relevant implementation or motivation in the details section. Use `git commit -s`
to add the `Signed-off-by` trailer from the configured Git identity.

Allowed commit types:

- `feat` — adding a new feature
- `fix` — fixing a bug
- `docs` — documentation changes
- `style` — code style changes that do not affect functionality, such as whitespace,
  formatting, or missing semicolons
- `refactor` — code refactoring that neither fixes a bug nor adds a feature
- `test` — adding or modifying tests
- `chore` — changes to the build process or auxiliary tooling that do not affect
  source code or tests
- `perf` — performance improvements
- `build` — changes to the build system or external dependencies
- `revert` — reverting a previous commit
- `ci` — changes to CI configuration files and scripts
- `deps` — dependency changes

Use a short, stable `scope` such as `core`, `launcher`, `cardputer`, `build`, or
`docs`. Keep unrelated changes in separate commits.

Example:

```text
feat(core): route boot input to Launcher

Adds coordinator-owned app lifecycle routing and shared service contracts.

Signed-off-by: Jane Doe <jane@example.com>
```

## Pull requests

GitHub pull requests use [.github/pull_request_template.md](.github/pull_request_template.md).
Describe the user-visible or architectural impact in **Summary**, and record the
commands or hardware checks actually completed in **Test**. If a check was not run,
state why and describe any remaining validation risk.
