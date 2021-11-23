# shell
Эмулятор терминала Linux
## Установка
Linux:

1. Скопируйте репозиторий на ваше устройство:

```sh
git clone git@github.com:exile8/shell.git
```
2. Выполните сборку программы (убедитесь, что вы находитесь в каталоге **shell**):

```sh
cd shell
make
```
## Примеры использования

* Выполнение команд linux без аргументов

```
user:/home/user/shell$ pwd
/home/user/shell
```

* Выполнение команд linux c аргументами

```
user:/home/user/shell$ ls -l -a
total 28
drwxrwxr-x  5 user user 4096 Nov  5 16:13 .
drwxrwxr-x 14 user user 4096 Oct 23 21:46 ..
drwxrwxr-x  2 user user 4096 Nov  5 16:13 bin
drwxrwxr-x  8 user user 4096 Nov  5 16:12 .git
-rw-rw-r--  1 user user   90 Nov  3 07:26 Makefile
-rw-rw-r--  1 user user 1274 Nov  5 16:12 README.md
drwxrwxr-x  2 user user 4096 Nov  5 16:13 source
```

* Пробелы и табуляции до, между и после аргументов игнорируются

```
user:/home/user/shell$    ls    -l  -a      
total 28
drwxrwxr-x  5 user user 4096 Nov  5 16:13 .
drwxrwxr-x 14 user user 4096 Oct 23 21:46 ..
drwxrwxr-x  2 user user 4096 Nov  5 16:13 bin
drwxrwxr-x  8 user user 4096 Nov  5 16:12 .git
-rw-rw-r--  1 user user   90 Nov  3 07:26 Makefile
-rw-rw-r--  1 user user 1274 Nov  5 16:12 README.md
drwxrwxr-x  2 user user 4096 Nov  5 16:13 source
```

* Перенаправление ввода

```
user:/home/user/shell$ cat input.txt
First line
Second line
Third line
user:/home/user/shell$ wc -l < input.txt
3
```

* Перенаправление вывода

```
user:/home/user/shell$ ls -a > output.txt
user:/home/user/shell$ cat output.txt
.
..
bin
.git
Makefile
output.txt
README.md
source
```

* Одновременное перенаправление ввода и вывода

```
user:/home/user/shell$ cat input.txt
some input info
for wc command
user:/home/user/shell$ wc -c < input.txt > output.txt
user:/home/user/shell$ cat output.txt
31
```

* Конвейер

```
user:/home/user/shell$ ls | sort
bin
Makefile
README.md
source
```

* Конвейер с перенаправлением ввода и вывода в файлы

```
user:/home/user/shell$ cat input.txt
apple
pie
pear
carrot
user:/home/user/shell$ sort < input.txt | rev | sort > output.txt
user:/home/user/shell$ cat output.txt
eip
elppa
raep
torrac
```

* **> filename** и **< filename** могут располагаться в любой части строки ввода

```
user:/home/user/shell$ cat input.txt
apple
pie
pear
carrot
user:/home/user/shell$ < input.txt sort > output.txt -r
user:/home/user/shell$ cat output.txt
pie
pear
carrot
apple
```

* Смена директории

```
user:/home/user/shell$ cd bin
user:/home/user/shell/bin$ cd
user:/home/user$ cd -
/home/user/shell/bin
user:/home/user/shell/bin cd ..
user:/home/user/shell$ cd /
user:/$ cd ~
user:/home/user$
```

* Фоновый режим

```
user:/home/user/shell$ gedit &
PID: 7556
user:/home/user/shell$ ls | sort | wc &
PID: 7559
user:/home/user/shell$       4       4      30
evince &
PID: 7560
user:/home/user/shell$
```
* Конвейреры **&&** и **||**

```
user:/home/user/shell$ vim hello.c && gcc hello.c -o hello && ./hello
hello!
user:/home/user/shell$ vim bad_code.c && gcc bad_code.c -o prog -Wall && ./prog
bad_code.c: In function ‘main’:
bad_code.c:4:28: error: expected ‘;’ before ‘i’
    4 |     for (int i = 0; i < 10. i++) {
      |                            ^~
      |                            ;
user:/home/user/shell$ cmd-does-not-exit || cmd-does-not-exist || pwd
exec: No such file or directory
exec: No such file or directory
/home/user/shell
user:/home/user/shell$ 
```

* Обработка сигнала **SIGINT** (ctrl + C)

```
user:/home/user/shell$ gedit
^C
user:/home/user/shell$ ^C
```

* Выход из программы

```
user:/home/user/shell$ exit
```

```
user:/home/user/shell$ quit
```

## История версий
* 1.0
    * Деление ввода на лексемы
* 2.0
    * Стандартный запуск программы
* 3.0
    * Перенаправление ввода и вывода
* 4.0
    * Конвейер из двух программ
* 5.0
    * Конвейер из n программ
* 6.0
    * Смена директории cd
* 7.0
    * Фоновый режим
* 8.0
    * Конвейеры && и ||
* 9.0
    * Обработка ctrl + C
