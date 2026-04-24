## Когда решил, что пора выгружать изменения в мастер-ветку:

```
git checkout master
git pull origin master
git merge --squash working
git commit -m "Реализация функционала из ветки working: краткое описание изменений"
git push origin master
git push origin working --force-with-lease
#git push origin working --force
git checkout working
```
Эта команда будет мгновенно приводить текущую ветку к состоянию сервера:
```
git fetch origin && git reset --hard origin/$(git rev-parse --abbrev-ref HEAD)
```
