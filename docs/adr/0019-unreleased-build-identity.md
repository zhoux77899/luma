# Unreleased Build identity is X.Y.Z.{sha}, not semver +build

About shows a release as X.Y.Z from the tagged version. An unreleased build appends the 7-character commit as a fourth dotted field so the string stays readable on the 240 px detail pane. CI previously used 0.1+g{sha}; that form is not the product identity.
