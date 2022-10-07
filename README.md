# ffmpeg_rtmp
这个项目的目标是从服务器拉取 rtmp 流并显示到野火 imx6ull 开发板上（正点原子开发板应该也可以用）。
# 准备工作
使用的是 ffmpeg 库，版本是 4.3.2。库的编译不多赘述，可参考下面的链接，需要注意的是其中的编译器参数需要改成交叉编译器的名字，如 arm-linux-gnueabihf-gcc。

[FFmpeg-4.3.2 嵌入式Linux交叉编译](https://blog.csdn.net/qq_29994663/article/details/115337049)

最终我们需要的就是 include 和 lib 两个文件夹，将其放在了根目录下。
# 说明
项目结构很简单，源文件放在了 src 目录下，main.c 包括屏幕的初始化等，getstream.c 负责处理数据并显示；include 和 lib 是上面提到的 ffmpeg 库，build 用来存放生成的可执行文件。

make 能得到可执行文件，程序依赖 lib 文件夹中的内容，我将其放在了 /usr/ffmpeg/lib 路径下，并在命令行输入 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/ffmpeg/lib，更好的方法是修改 /etc 下的配置文件，但是我是使用了 busybox 生成的系统，这个功能不完善，所以用 export 命令替代，在重启系统后需要重新输入。
