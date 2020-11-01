# Contributing to TSDuck development   {#contributing}
[TOC]

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

The first requirement is to create a GitHub account, if you do not already have one.

Initially, create your own fork of the TSDuck repository.
Go to the TSDuck [reference repository](https://github.com/tsduck/tsduck)
and click on the "Fork" button.

Clone your GitHub forked repository on your development system.
Use one of the two following commands.
~~~
git clone https://github.com/USERNAME/tsduck.git
git clone ssh://git@github.com/USERNAME/tsduck.git
~~~
In the first case, you will need to provide your password each time you push
to GitHub. In the second case, you need to first upload your SSH public key to
GitHub and then simply push without password.

You may want to track more precisely the master branch of the reference
repository. See more details in the
[above mentioned article](https://gist.github.com/Chaser324/ce0505fbed06b947d962).

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
