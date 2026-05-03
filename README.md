# Portfolio

## env:

### Hardware: TX2024 Laptop
    CPU: AMD HX370 + GPU: 4060 laptop

### OS: Windows 11

### IDE: Rider

### Basic Config:
#### Github:
Rider本地新建C++项目

git branch -M main

git push -u origin main

0.1:
fatal: unable to access 'https://github.com/FrostDawnNico/PFL.git/': Failed to connect to github.com port 443 after 21131 ms: Could not connect to server

git config --global --unset http.proxy

git config --global --unset https.proxy

git config --global http.proxy http://127.0.0.1:7897 (手动设置代理-代理IP地址:端口)