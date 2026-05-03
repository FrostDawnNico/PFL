# Setup Progress:

## GitHub:
1. Rider本地新建C++项目

2. Git-在GitHub上共享项目

遇到prob1失败后手动建立方法：

1. 在GitHub建立空仓库

2. git remote add origin https://github.com/FrostDawnNico/PFL.git）
（链接远程仓库）

3. branch -M main
（重命名当前分支）

4. git push -u origin main
（设置上游跟踪关系，main-origin/main）

### Prob 1:
fatal: unable to access 'https://github.com/FrostDawnNico/PFL.git/': Failed to connect to github.com port 443 after 21131 ms: Could not connect to server
### Solu
git config --global --unset http.proxy

git config --global --unset https.proxy

git config --global http.proxy http://127.0.0.1:7897 (手动设置代理-代理IP地址:端口）

