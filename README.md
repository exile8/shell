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
$ pwd
/home/user/shell
```

* Выполнение команд linux c аргументами

```
$ ls -l -a
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
$    ls    -l  -a      
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
$ cat input.txt
First line
Second line
Third line
$ wc -l < input.txt
3
```

* Перенаправление вывода

```
$ ls -a > output.txt
$ cat output.txt
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
$ cat input.txt
some input info
for wc command
$ wc -c < input.txt > output.txt
$ cat output.txt
31
```

* Конвейер

```
$ ls | sort
bin
Makefile
README.md
source
```

* Конвейер с перенаправлением ввода и вывода в файлы

* Выход из программы

```
$ exit
```

```
$ quit
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
