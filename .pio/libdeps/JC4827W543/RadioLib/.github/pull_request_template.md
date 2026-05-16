## Pull request template
Thank you for taking the time to contribute to RadioLib development!
To keep this library organized, please follow these rules.

1. Make sure the the code in your PR is tested and that you understand all its impacts.
2. Ensure that all CI actions pass - PRs with failed CI will not be merged. CI actions run automatically for every commit pushed to the PR and test the following:
  a. Compilation for Arduino, ESP-IDF and on Raspberry Pi
  b. Runtime test on Raspberry Pi
  c. GitHub CodeQL check
  d. Cppcheck static code scan
3. Follow code style guidelines in [CONTRIBUTING.md](https://github.com/jgromes/RadioLib/blob/master/CONTRIBUTING.md)
4. Heads up - all PRs undergo review, during which you may be asked to correct or change some things. The purpose of this review is to keep regressions and bugs at the minimum, and to keep consistent coding style. Please take them as constructive criticism from people who may have a different point-of-view than you do.

After addressing/accepting the points above, delete the contents of this template and replace it with text explaining what is the goal of your PR, why you want to add it to the upstream and what are the foreseen impacts. Once again, thank you for taking the time to contribute!
