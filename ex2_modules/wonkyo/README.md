# Writing a simple linux kernel module
이 문서는 기본적인 Kernel module 을 작성하는 법을 다루고 있습니다.

### 준비물
* 이 문서를 읽는 예상 독자는 기본적인 linux command 와 C 언어에 어느 정도 익숙한 사람입니다.
* Ubuntu 16.04 or any other distributions (이 글은 Ubuntu 에 기반하여 작성되었습니다.)

### Getting started
가장 먼저 원하는 디렉토리를 만들어서 module 코드를 담을 `simple.c` 를 작성합니다. 

```c
/* simple.c */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wonkyo Choe");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

static int __init simple_init(void) {
    printk(KERN_INFO "Hello, World!\n");
    return 0;
}

static void __exit simple_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(simple_init);
module_exit(simple_exit);
```

위의 코드는 module 이 정상적으로 동작하기위한 최소한의 필요한 함수와 변수들을 담고 있습니다.
다음 사항들은 코드에 있는 함수 및 변수들에 대한 설명입니다.
* `<linux/->` header file 은 Linux kernel development 에 필요한 것들을 담고 있습니다.
* `MODULE_LICENSE` 는 말 그대로 module license 에 관한 내용이며 자세한 사항들은 아래 명령문을 통해 확인할 수 있습니다.

    ```
    grep "MODULE_LICENSE" -B 27 /usr/src/linux-headers-`uname -r`/include/linux/module.h
    ```
* `init`(loading) 와 `exit`(unloading) 함수를 사용하였습니다.
* `moudle_init` 을 통해 module 을 kernel 에 loading 할 수 있고 `module_exit` 을 통해 moudle 을 unloading 할 수 있습니다.
이 함수 덕분에 개발자들은 임의의 `init` 함수와 `exit` 함수를 사용할 수 있습니다.


다음으로 코드를 작성하였다면 이 코드를 compile 해서 linux 가 받아들일 수 있는 파일로 만들어야 합니다.
그런데, 일반적인 `gcc` 는 실행파일을 만들어내기 때문에 사용해선 안 되고 또한 `gcc` 를 사용할 경우 
`<linux>` header 의 경로를 찾지 못 해 compile 을 제대로 하지 못 합니다. 
따라서 다음과 같은 `Makefile` 을 작성하여 kernel 이 가지고 있는 build processing 을 통해 module 을 compile 해야 합니다.  [Why?](https://stackoverflow.com/questions/8062601/linux-module-h-no-such-file-or-directory)


```shell
obj-m += simple.o

all:
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

```

`Makefile` 을 작성 후 `make all` 을 실행하여 build 를 완료하였다면, `simple.ko` 와 같은 파일이 생성됩니다.
생성된 파일은 module 파일로 다음과 같은 명령어를 통해 kernel 에 적용됩니다.

```shell
$ sudo insmod simple.ko

```

만약 module 을 kernel 에서 제거하고 싶다면 아래와 같은 명령어를 입력합니다.

```shell
$ sudo rmmod simple
```

module 을 삽입하고 제거할 때 작성한 코드에 의해 `printk` 로 남겨지는 텍스트들이 있을텐데
이러한 글자들은 kernel log 에 남겨지게 됩니다. 이러한 문자를 확인하기 위해선 다음 두 개의 명령어를 입력하면 됩니다.

```shell
# #1
$ sudo dmesg

# #2
$ lsmod | grep "simple"
```

만약 module 의 작동유무만 확인하고 싶다면 위의 명령어들을 전부 묶어서 `Makefile` 에 test 케이스를 만듭니다.

```shell
test:
    sudo dmesg -C
    sudo insmod simple.ko
    sudo rmmod simple.ko
```

실행!
```shell
$ make test
```

### A Bit More Interesting
**Passing command line Arguement to module**
아래의 코드는 module 을 kernel 에 삽입할 때 command line 에 argument 가 있을 경우
이 argument 값들을 전달해주도록 하는 코드입니다.

```c
/* simple-5.c */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wonkyo Choe");
static short int myshort = 1;
static int myint = 420;
static long int mylong = 9999;
static char *mystring = "blah";
static int myintArray[2] = { −1, −1 };
static int arr_argc = 0;
/*
 * module_param(foo, int, 0000)
 * The first param is the parameters name
 * The second param is it's data type
 * The final argument is the permissions bits,
 * for exposing parameters in sysfs (if non−zero) at a later stage.
 */
module_param(myshort, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(myshort, "A short integer");
module_param(myint, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(myint, "An integer");
module_param(mylong, long, S_IRUSR);
MODULE_PARM_DESC(mylong, "A long integer");
module_param(mystring, charp, 0000);
MODULE_PARM_DESC(mystring, "A character string");
/*
 * module_param_array(name, type, num, perm);
 * The first param is the parameter's (in this case the array's) name
 * The second param is the data type of the elements of the array
 * The third argument is a pointer to the variable that will store the number
 * of elements of the array initialized by the user at module loading time
 * The fourth argument is the permission bits
The Linux Kernel Module Programming Guide
Chapter 2. Hello World 12
 */
module_param_array(myintArray, int, &arr_argc, 0000);
MODULE_PARM_DESC(myintArray, "An array of integers");

static int __init simple_5_init(void)
{
    int i;
    printk(KERN_INFO "Hello, world\n=============\n");
    printk(KERN_INFO "myshort is a short integer: %hd\n", myshort);
    printk(KERN_INFO "myint is an integer: %d\n", myint);
    printk(KERN_INFO "mylong is a long integer: %ld\n", mylong);
    printk(KERN_INFO "mystring is a string: %s\n", mystring);
    for (i = 0; i < (sizeof myintArray / sizeof (int)); i++)
    {
        printk(KERN_INFO "myintArray[%d] = %d\n", i, myintArray[i]);
    }
    printk(KERN_INFO "got %d arguments for myintArray.\n", arr_argc);
    return 0;
}

static void __exit simple_5_exit(void)
{
    printk(KERN_INFO "Goodbye, world\n");
}
module_init(simple_5_init);
module_exit(simple_5_exit);
```

`make` 를 통해 build 후 작동여부를 확인하고 싶다면 아래와 같은 명령어를 입력하면 됩니다.

```shell
$ insmod simple-5.ko mystring="bebop" mybyte=255 myintArray=-1
# expected result
mybyte is an 8 bit integer: 255
myshort is a short integer: 1
myint is an integer: 20
mylong is a long integer: 9999
mystring is a string: bebop
myintArray is −1 and 420

```

## Reference
* [Writing a simple linux kernel module](https://blog.sourcerer.io/writing-a-simple-linux-kernel-module-d9dc3762c234) 
* [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf)



