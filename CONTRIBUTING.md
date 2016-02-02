Contributing to EbbRT
=====

Thank you for your interest in contributing to EbbRT.

## Pull Requests

Pull requests are the primary mechanism we use to change Rust. GitHub itself
has some [great documentation][pull-requests] on using the Pull Request
feature. We use the 'fork and pull' model described there.

[pull-requests]: https://help.github.com/articles/using-pull-requests/

Please make pull requests against the `master` branch.

## Git Commit Guidelines

We have very precise rules over how our git commit messages can be
formatted.  This leads to **more readable messages** that are easy to
follow when looking through the **project history**.

### Commit Message Format
Each commit message consists of a **header**, a **body** and a
**footer**.  The header has a special format that includes a **type**,
a **scope** and a **subject**:

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

The **header** is mandatory and cannot be longer than 50
characters. Every line within the body and footer should be no more
than 72 characters.

### Revert
If the commit reverts a previous commit, it should begin with `revert:
`, followed by the header of the reverted commit. In the body it
should say: `This reverts commit <hash>.`, where the hash is the SHA
of the commit being reverted.

### Type
Must be one of the following:

* **feat**: A new feature
* **fix**: A bug fix
* **docs**: Documentation only changes
* **style**: Changes that do not affect the meaning of the code
  (white-space, formatting, missing semi-colons, etc)
* **refactor**: A code change that neither fixes a bug nor adds a feature
* **perf**: A code change that improves performance
* **test**: Adding missing tests
* **chore**: Changes to the build process or auxiliary tools and
  libraries such as documentation generation

### Scope
The scope could be anything specifying place of the commit change. For
example `$net`, `$event`, `$build`, etc...

### Subject
The subject contains succinct description of the change:

* use the imperative, present tense: "change" not "changed" nor "changes"
* don't capitalize first letter
* no dot (.) at the end

### Body
Just as in the **subject**, use the imperative, present tense:
"change" not "changed" nor "changes".  The body should include the
motivation for the change and contrast this with previous behavior.

### Footer
The footer should contain any information about **Breaking Changes**
and is also the place to reference GitHub issues that this commit
**Closes**.

TEST
