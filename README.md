# Opengl Stanford Bunny Demo

计算机图形学课程作业，用 OpenGL 渲染 Stanford Bunny 模型。

使用的第三方库有 [Dear ImGui](https://github.com/ocornut/imgui)，[GLFW](https://www.glfw.org/)，[GLEW](https://github.com/nigels-com/glew)。

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
- [x] 反走样：GLFW 下成功
- [x] 跨平台支持
  - [x] Apple
  - [x] Linux
  - [x] MSYS2
- [x] 自定义 shader
- [x] 面向对象重构
- [ ] 提供编译时配置选择使用 SDL2 或 GLFW
- [ ] 使用 select mode 外的方式实现拾取
- [ ] 使用 PolygonMode 外的方式实现线框绘制
- [ ] 升级 OpenGL 版本：可在运行时选择
  - [x] 使用 glm 代替 GLU 的矩阵变换函数
  - [x] 使用顶点属性向着色器传递数据
  - [x] OpenGL 3.2 core profile
  - [ ] OpenGL ES 2.0
  - [ ] 包装绘制对象，切分定点属性设置步骤
