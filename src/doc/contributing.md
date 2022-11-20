# Contributing to TSDuck   {#contributing}

TSDuck development is managed using Git.
The [reference repository](https://github.com/tsduck/tsduck) is on GitHub.

Code contributions from external developers are welcome and will be reviewed
(without guaranteed response time however). Contributions shall be submitted
using [pull requests on GitHub](https://github.com/tsduck/tsduck/pulls)
exclusively.

This documentation page summarizes the main actions to help developers and
integrators to work with pull requests. This is the minimal set of actions.
More details can be found on
[GitHub documentation](https://help.github.com/articles/about-pull-requests/).
Several articles also describe the GitHub standard fork & pull request workflow.
We specifically [recommend this one](https://gist.github.com/Chaser324/ce0505fbed06b947d962).

# Transparency of contributions   {#transparency}

All commits in a Git pull request shall have a clear and transparent identification
of the author. The author name shall be the true first and last names of the contributor.
No pseudo or other forms or anonymity is allowed. Preferably (although not required),
the author's e-mail should be a real address where the contributor can be contacted.

This requirement for transparency is not arbitrary. There is a reason for it.
The world of Digital TV is roughly divided in industries, service providers,
TV operators and pirates. A technical toolbox such as TSDuck is useful to everyone, equally.
But it must be clear to everyone that TSDuck is made by engineers for engineers.
TSDuck shall remain a fully transparent project: open source, identified web sites,
identified authors and contributors.

In the world of Pay-TV, anonymity equals piracy. This may seem unfair but this
is the way it is perceived by the industry. So, to maintain the trust in TSDuck,
let's keep anonymity away from it. We hope that all contributors understand this
position.

# Contributor workflow   {#contributor}

## Initial setup

The first requirement is to create a GitHub account, if you do not already have one.

Initially, create your own fork of the TSDuck repository.
Go to the TSDuck [reference repository](https://github.com/tsduck/tsduck)
and click on the "Fork" button.

Clone your GitHub forked repository on your development system.
Use one of the two following commands.
~~~
git clone https://USERNAME@github.com/USERNAME/tsduck.git
git clone git@github.com:USERNAME/tsduck.git
~~~
In the first case, you will need to provide a GitHub personal access token each
time you push to GitHub. In the second case, you need to first upload your SSH
public key to GitHub and then simply push without password.

You may want to track more precisely the master branch of the reference
repository. See more details in the
[above mentioned article](https://gist.github.com/Chaser324/ce0505fbed06b947d962).

## Contributing code

To facilitate merging, each contribution should be provided in a specific
branch. Let's call it `newfeature` here:
~~~
git branch newfeature
git checkout newfeature
~~~
Then, do your coding work, commit modifications and push the work to GitHub:
~~~
git push origin newfeature
~~~

Finally, create the pull request. Go to your forked repository on GitHub,
something like `https://github.com/USERNAME/tsduck`, and select the branch
`newfeature`. Select the "Pull requests" tab and click on the green button
"New pull request". Select the branch for the pull request and click on
"Create pull request".

The pull request is transmitted to the project main repository `tsduck/tsduck`.
The Continuous Integration (CI) process is automatically started on your code.
During this CI process, the code is compiled and tested on Linux, macOS and
Windows using various compilers and variants of the C++ standard. Then, all
TSDuck tests are run on your code.

In case of failure of the CI job, you will be notified by mail and the
pull request is retained on hold. You may then review the failures,
update your code, commit and push on your branch again. The new
commits are added to the pull request and the CI job is run again.

Most users work in one environment, one operating system, one compiler.
Even if the code works in this environment, it may fail to build or run
in another environment, operating system or compiler. This is why the CI
process is useful because you can review the impact of your modifications
in other environments.

## Testing your code

Before pushing your code and creating the pull request, you will test your
code. Additionally, when your add new features or support for new signalization,
tables or descriptors, it is recommended to update the TSDuck test suite.

See the [testing description](testing.html) for more details.

To update the TSDuck test suite according to your new code, fork the
[tsduck-test](https://github.com/tsduck/tsduck-test) repository, update it
and create pull requests on this repo using the same method as the main
[tsduck](https://github.com/tsduck/tsduck) repository.

Important: Pay attention to the interactions between the `tsduck` and `tsduck-test`
repositories. The `tsduck-test` repository contains tests and reference
outputs for those tests. When you update the TSDuck code, the test reference
output may need to be updated accordingly. You do that in your fork of
the `tsduck-test` repository. When you create a pull request on the main
`tsduck` repository, the CI job checks the origin of the pull request.
In your case, this is your `USERNAME/tsduck` forked repository. The CI
job checks if you also have a `USERNAME/tsduck-test` forked repository.
If it exists, it is used to run the test suite. If you do not have
a fork of the test repository, the reference `tsduck/tsduck-test`
repository is used.

Consequently, the recommended workflow depends on the type of code
contribution you provide.

- If you provide a simple code update which has no impact on the test suite,
then you should fork the `tsduck/tsduck` repository only. Your code will
be tested against the `tsduck/tsduck-test` repository to make sure it does
not break the project.

- If your contribution is more substantial and needs an update of the test
suite, then you need to fork the `tsduck/tsduck` and `tsduck/tsduck-test`
repositories. Once your code and tests are complete, create the commits
and push the two repositories. At the end, create the pull requests on
the two repositories. The CI job for the `tsduck` repository will then
use your `USERNAME/tsduck-test` repository for the test suite. If all
tests pass on all operating systems, your contributions in `tsduck` and
`tsduck-test` will be merged.

One last point: If you maintain your fork of `USERNAME/tsduck-test`,
be sure to keep it synchronized with the reference `tsduck/tsduck-test`
repository because your `USERNAME/tsduck-test` will always be used in
your CI jobs. If one day, you submit a small code update which did not
need any update in the test suite and your `USERNAME/tsduck-test` is
not up-to-date, your CI job may fail.

# Integrator workflow   {#integrator}

On your local development system, configure your TSDuck development git
repository to track all pull requests. In the file `.git/config`, add the
following line in section `[remote "origin"]`:
~~~
fetch = +refs/pull/*/head:refs/pull/origin/*
~~~
To integrate a pull request number NNN, fetch it in a local branch named `NNN`:
~~~
git fetch origin
git checkout -b NNN pull/origin/NNN
~~~
To merge the pull request into the `master` branch:
~~~
git checkout master
git merge NNN
~~~
Additional review and fix may be necessary before pushing the contribution.

Specifically, if the contribution brings new features, be sure to document
them in the TSDuck user's guide. New features and bug fixes should also be
documented in CHANGELOG.txt.
