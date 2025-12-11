# Bookstore - 2025 总体设计文档

文档作者 ：叶博文

Part I 程序功能概述
---

实现一个书店管理系统，模拟其各种功能：
1. 用户功能
- 登录账户
- 注销账户
- 注册账户
- 修改密码
- 创建账户
- 删除账户

2. 图书管理功能
- 检索图书
- 购买图书
- 选择图书
- 修改图书信息
- 图书进货

3. 工作日志功能
- 财务记录查询
- 生成财务记录报告
- 生成全体员工工作情况报告
- 生成日志

---

Part II 主体逻辑说明
---
    把用户和图书封装为两个类，并使用块状链表的结构存储其数据，建立用户数据库和图书数据库。
    把交易信息和工作日志写成另外两个类，建立数据库以存储对应信息。
    主程序读入操作并分词，通过对应类的函数进行实际操作，从而模拟书店的管理

---

Part III 代码文件结构
---

```
Bookstore - 2025
├── include
│   ├── book.h
│   ├── book_database.h
│   ├── log.h
│   ├── log_database.h
│   ├── user.h
│   └── user_database.h
├── src
│   ├── book.cpp
│   ├── log.cpp
│   ├── main.cpp
│   └── user.cpp
```


main函数与各个类的关系：
（具体类参见 **Part VI**）
在main函数中创建UserDatabase类作为用户的数据库
在main函数中创建BookDatabase类作为用户的数据库
在main函数中创建LogDatabase类作为用户的数据库
在main函数中创建DealDatabase类作为用户的数据库
在main函数中读取输入并分词，调用上述各个类中的函数完成书店管理系统的基本功能

---

Part IV 功能设计
---
1. 用户功能
- 登录账户：
```{Privilege} su [UserID] ([Password])?```
> 在UserDatabase中查询账户是否存在，若存在且密码正确或权限高于当前账户则将账户压入登录栈,修改当前权限；不存在或密码错误则操作失败。
- 注销账户
```{Privilege} logout```
> 若登录栈非空，则把栈顶账户弹出，修改当前权限；否则操作失败。
- 注册账户
```{Privilege} register [UserID] [Password] [Username]```
> 在UserDatabase中查询用户名是否已经存在，若不存在则插入这个新账户，否则操作失败。
- 修改密码
```{Privilege} passwd [UserID] ([CurrentPassword])? [NewPassword]```
> 在UserDatabase中查询账户是否存在，若存在且原密码正确则修改为新密码；否则操作失败。
- 创建账户
```{Privilege} useradd [UserID] [Password] [Privilege] [Username]```
> 在UserDatabase中查询账户是否存在，若存在且当前权限高于待创建的账户权限则创建成功；否则存在失败。
- 删除账户
```{Privilege} delete [UserID]```
> 在UserDatabase中查询账户是否存在，若账户存在且不在登录栈中则删除账户；否则操作失败。

![](https://notes.sjtu.edu.cn/uploads/upload_2b4b7998b798ba1e08ceaa283063cdf9.png)


2. 图书功能
- 检索图书
```{Privilege} show (-ISBN=[ISBN] | -name="[BookName]" | -author="[Author]" | -keyword="[Keyword]")?```
> 根据输入参数调用对应函数在BookDatabase中查询满足条件的图书并按照ISBN升序输出
- 购买图书
```{Privilege} buy [ISBN] [Quantity]```
> 在BookDatabase中查询是否存在图书，若存在且数量正确则减少对应库存；否则操作失败。
- 选择图书
```{Privilege} select [ISBN]```
> 在BookDatabase中查询是否存在图书，若存在则把当前选择图书更新；否则创建新图书并插入数据库中。
- 修改图书信息
```{Privilege} modify (-ISBN=[ISBN] | -name="[BookName]" | -author="[Author]" | -keyword="[Keyword]" | -price=[Price])+```
> 检查是否为当前选择图书，若是且参数正确则根据输入参数调用对应函数修改对应信息；否则操作失败；
- 图书进货
```{Privilege} import [Quantity] [TotalCost]```
> 检查当前是否有选择图书，若有且购入数量和交易总额正确则更新库存并把支出插入DealDatabase中；否则操作失败

![](https://notes.sjtu.edu.cn/uploads/upload_3b7131bd9a7df4fec1bdab3160584d0a.png)


3. 工作日志功能
- 财务记录查询
```{7} show finance ([Count])?```
> 若输入交易笔数正确则在DealDatabase中遍历最后指定笔数的交易信息，输出交易总额；否则操作失败。
- 生成财务记录报告
```{7} report finance```
> 遍历DealDatabase中所有的交易记录，输出操作者、操作类型、具体收支。
- 生成全体员工工作情况报告
```{7} report employee```
> 遍历整个工作日志找出权限为{3}的用户的操作，输出操作者、操作类型、具体收支（如果是交易的操作）。
- 生成日志
```{7} log```
> 遍历整个工作日志，输出操作者、操作类型、具体收支（如果是交易的操作）。

![](https://notes.sjtu.edu.cn/uploads/upload_3c607d934d7d914285b0e482c4e8e2c4.png)

---

Part V 数据库设计
---

1. 用户数据库
- 需要存储的数据：所有用户的账户，每一个账户以键值对形式，其中`UserID`为key，`Password`,`Privilege`,`Username`为value
  
- 存储方式：块状链表

2. 图书数据库
- 需要存储的数据：所有库存的图书，每一本图书以键值对形式，其中`ISBN`为key，`Bookname`,`Author`,`Keyword`,`Price`，`Quantity`为value

- 存储方式：块状链表
3. 日志数据库
- 需要存储的数据：按照操作时间存入所有系统执行的操作，包括`UserID`,`Operationtype`,`Income`，`Expense`。

- 存储方式：直接写入文件
4. 交易数据库 
- 需要存储的数据：一个表示收支的double型变量`Profit`，正数代表收入，负数代表支出

- 存储方式：直接写入文件

---

Part VI 类、结构体设计
---
### 类
`User`类：用于封装用户信息 

`UserOperation`类：用于实现对于用户账户的操作，如插入、删除、修改等

`UserDatabase`类：用于存储用户的账户信息

`Book`类：用于封装图书信息

`BookOperation`类：用于实现对于图书信息的操作，如插入、删除、修改等

`BookDatabase`类：用于存储图书信息

`Log`类：用于封装工作日志信息

`LogDatabase`类：用于存储工作日志信息

`DealDatabase`类：用于存储收入支出等信息

### 结构体

`Block`结构体：用于实现块状链表节点

---

Part VII 其他补充说明
---

暂无（以后可能补充）

---