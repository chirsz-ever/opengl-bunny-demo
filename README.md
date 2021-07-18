# Opengl Stanford Bunny Demo

计算机图形学课程作业，用 OpenGL 渲染 Stanford Bunny 模型。

使用的第三方库有 [Dear ImGui](https://github.com/ocornut/imgui)，[SDL2](https://www.libsdl.org/)，[GLEW](https://github.com/nigels-com/glew)。

当前代码基于 OpenGL 2.1。

模型数据来自 [Objects in OBJ format](https://www.prinmath.com/csci5229/OBJ/index.html)。

## 编译方法

执行

```shell
$ make
```

编译成功后执行

```shell
$ ./bunny-ui
```

以运行。

## 实现的功能

- 窗口左侧为 UI 界面，可设置各种属性，窗口右侧为渲染区域，显示渲染结果；窗口可缩放；
- 在渲染区域用鼠标左键左右拖动模型旋转，鼠标右键上下拖动改变俯仰视角；
- 有两个光源，在左侧控制窗口处可设置其光照属性和位置；
- 左侧控制窗口还可设置全局环境光、模型材质、线框显示、显示光源位置等；
- 可开启拾取功能，拾取模型顶点或面片。

## TODO

- [x] 用三角面片绘制球，先生成数据再用 glDrawElements 绘制
- [ ] 反走样
- [ ] 跨平台支持
- [x] 自定义 shader
- [ ] 升级 OpenGL 版本
  - [x] 使用 glm 代替 GLU 的矩阵变换函数
  - [ ] 使用顶点属性向着色器传递数据
  - [ ] 使用 select mode 外的方式实现拾取
