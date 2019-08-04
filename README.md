![](https://github.com/ex-rip/ehdown/raw/master/ex.jpg)

# e站搜索工具（本地）

使用e站的数据库

[数据库](https://pan.baidu.com/s/18xSrYlx29NkSYfby3ct01A) 密码 p0q1

[程序](https://github.com/ex-rip/EHentaiSearch/releases/latest/download/release.7z)

[QT依赖](https://github.com/ex-rip/EHentaiSearch/releases/download/1.0.3/depend.7z) 如果安装了QT并添加到PATH，就不需要这个包

## 使用

![](https://github.com/ex-rip/EHentaiSearch/raw/master/ui.png)

#### 标题

按标题搜索，包括英文标题或日文标题

#### 标签

格式：类:值

* id：符号 数值
	* id:>4
	* id:<=10
* category：类别
* rating：评分 同id
* artist：艺术家
* group：组织
* parody：原作
* language：语言
* misc：杂项标签
* female：女性
* male：男性
* character：人物

## ini配置

### thumb 封面

#### enable

是否启用 true/false

#### prefix suffix

封面链接的前缀、后缀

#### password

aes加密的密码

aes加密使用__256bit__

### database 数据库

#### path

数据库路径

#### limit

每一页显示结果数