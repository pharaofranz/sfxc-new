#!/bin/sh
svnmaxrev=2147483647
notsvn="Unversioned directory"
nosvnbranch="GIT"
#if [ -e command -v svnversion &> /dev/null ]
if command -v svnversion 2>&1 1>/dev/null
then
  svnversion=`svnversion -nc ..` 2>/dev/null
  if [ "${svnversion}" = "${notsvn}" ]
  then
    # Current tree is not managed by svn
    svnversion=${svnmaxrev}
    svnbranch=${nosvnbranch}
  else
    svnbranch=TRUNK
  fi
else
    # svn is not installed on the system
    svnversion=${svnmaxrev}
    svnbranch=${nosvnbranch}
fi
if command -v git 2>&1 1>/dev/null
then
  gitdescribe=`git describe --all --long 2>/dev/null`
  if [ $? -ne 0 ]
  then
    # Not a git repository
    correlatorversion="${svnversion}"
  else
    correlatorversion=`echo ${gitdescribe} | sed -e 's/^heads\///'`
  fi
else
  # Git is not installed
  correlatorversion="${svnversion}"
fi

echo "const char *SVN_VERSION=\"${svnversion}\";"
echo "const char *SVN_BRANCH=\"${svnbranch}\";"
echo "const char *CORRELATOR_VERSION=\"${correlatorversion}\";"

