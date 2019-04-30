# triqs_tprf

An example application using cpp2py and triqs
---------------------------------------------

To use this skeleton for a new triqs application, the following steps are necessary:

* Create a repository, e.g. https://github.com/myuser/mynewapp

* Run the following commands (replacing myuser and mynewapp accordingly)

```bash
git clone https://github.com/triqs/triqs_tprf --branch unstable mynewapp
cd mynewapp
git remote set-url origin https://github.com/myuser/mynewapp
find . -type f | grep -v .git | xargs sed -i 's/triqs_tprf/mynewapp/g; s/TPRF/MYNEWAPP/g'
find . -type d | grep -v .git | xargs rename triqs_tprf mynewapp
find . -type f | grep -v .git | xargs rename triqs_tprf mynewapp
git add -A
git commit -m "Create mynewapp from github.com/triqs/triqs_tprf skeleton"
git push
```

Depending on which version of the rename command you are using you may
need to replace the aforementioned rename commands as follows

```bash
find . -type d | grep -v .git | xargs rename 's/triqs_tprf/mynewapp/'
find . -type f | grep -v .git | xargs rename 's/triqs_tprf/mynewapp/'
```

If you prefer to use the SSH interface to the remote repository,
replace the http link accordingly

```
https://github.com/myuser/mynewapp   -->   git@github.com:myuser/mynewapp
```
