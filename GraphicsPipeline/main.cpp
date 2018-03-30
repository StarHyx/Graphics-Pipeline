#include <vector>
#include <stdio.h>
#include <time.h>
//引入所需的OpenGL工具库，GLUT和glm（数学工具库）
#include <GLUT/GLUT.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//引入文件图像加载库stb_image.h,用于解析图片格式
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//设置命名空间
using namespace std;
using namespace glm;

//宏定义max,min,调用glm库中函数时需要
#define  min(x,y) (x)<(y)?(x):(y)
#define  max(x,y) (x)>(y)?(x):(y)

//设置与光源有关的初始参数，
float lightPos1[] = { 0, 5, 0, 1 };//光源位置
float ambient[] = { 0.1, 0.1, 0.1, 1.0 };
float diffuse[] = { 1, 1, 1, 1.0 };//散射光参数
float specular[] = { 1, 1, 1, 1.0 };//镜面光参数

//vec3 三维向量
vec3 cameraPos = { 0,10,-10 };//设定摄像机位置
vec3 lookDir = {0,-1,1};//设定摄像机旋转
vec3 up = {0,1,0};//定义上向量

int lastX = 0, lastY = 0;
float rotateX = 0, rotateY = 0;//X,Y方向旋转参数（绕Z轴旋转）

bool keyDown[] = {false,false,false ,false };		//wsad

//设置与纹理有关的参数
GLuint boxTex = 0;//读取container.png
GLuint floorTex = 0;//读取floor.jpg
GLuint earthTex = 0;//读取earth.jpg
GLuint pepsiTex = 0;//读取pepsi.jpg

mat4 projectionMat;//定义位置矩阵-四维
mat4 viewMat;//定义观察矩阵--四维

//设定物体数据结构
struct ObjectData
{
	vec3 position;//位置参数
	float rotateX, rotateY;//X,Y方向旋转参数
	bool lighting;//光照参数
};
ObjectData objectData[] = {
	{ { -3,3,0 },0,0,true },	//cube-立方体
	{ { 0,3,0 },0,0,true },		//sphere-球体
	{ { 3,3,0 },0,0,true },		//cylinder-圆柱体
};

int selectedIndex = -1;
bool cameraMoving = false;
bool objectRotating = false;

//loadTexture函数，将图片作为纹理绘制到物体上
//GL_TEXTURE_MAG_FILTER: 放大过滤
//GL_LINEAR: 线性过滤, 使用距离当前渲染像素中心最近的4个纹素加权平均值.
//GL_TEXTURE_MIN_FILTER: 缩小过滤
//GL_TEXTURE_MIN_FILTER: 缩小过滤
//GL_REPEAT：重复，图象在表面上重复出现。忽略纹理坐标的整数部分，并将纹理图的拷贝粘贴在物体表面上,做到无缝连接。
GLuint loadTexture(const char *file)
{
	GLuint tex=0;
	int w, h, c;
	unsigned char *data = stbi_load(file, &w, &h, &c, 3);//读入图片文件到data
	if (data)
	{
		glGenTextures(1, &tex);//为读入纹理分配索引值
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//在S方向贴图
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//在T方向贴图
		free(data);
	}
	return tex;
}


//绘制底面平面
//GL_QUADS：绘制由四个顶点组成的一组单独的四边形
//glVertex3f：指定顶点坐标
//glTexCoord2f：指定纹理映射坐标
void drawPlane()
{
	glPushMatrix();//推送矩阵堆栈
    glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glTexCoord2f(0,0);
	glVertex3f(-1, 0, -1);
	glTexCoord2f(0, 1);
	glVertex3f(1, 0, -1);
	glTexCoord2f(1, 1);
	glVertex3f(1, 0, 1);
	glTexCoord2f(1, 0);
	glVertex3f(-1, 0, 1);
	glEnd();
	glPopMatrix();
}

//绘制立方体
//glTranslatef：沿x,y,z方向平移
//glRotatef(angle, x, y, z )：将当前坐标系以a( x, y, z )向量为旋转轴旋转angle角
void drawCube()
{
	//绘制上底面
    glPushMatrix();
	glTranslatef(0, 1, 0);
	drawPlane();
	glPopMatrix();

	//绘制下底面
    glPushMatrix();
	glTranslatef(0, -1, 0);
	glRotatef(180, 1, 0, 0);
	drawPlane();
	glPopMatrix();

    //绘制四个侧面
	glPushMatrix();
	for (int i=0;i<4;++i)
	{
		glRotatef(90 * i, 0, 1, 0);
		glPushMatrix();
		glTranslatef(0, 0, 1);
		glRotatef(90, 1, 0, 0);
		drawPlane();
		glPopMatrix();
	}
	glPopMatrix();
}

//绘制球体
void drawSphere()
{
	glPushMatrix();
	glBegin(GL_QUADS);

    //经纬线上的绘制精度
	int section = 50;
	int slice = 50;

    //在给定精度下进行循环绘制
    //在i的循环下，每次画出一个小平面，组成一个环
    //进入j循环，利用许多小环拼接出球体
	for (int j = 0; j < slice; ++j)
	{
		for (int i = 0; i < section; ++i)
		{
			float phi1 = radians(360.0f / section*i), phi2 = radians(360.0f / section*(i + 1));
			float theta1 = radians(180.0f / slice*j-90.0), theta2 = radians(180.0f / slice*(j+1)-90.0);

			glTexCoord2f(1 - i / (float)section, 1-j / (float)slice);
			glNormal3f(cos(theta1)*cos(phi1), sin(theta1), cos(theta1)*sin(phi1));
			glVertex3f(cos(theta1)*cos(phi1), sin(theta1), cos(theta1)*sin(phi1));

			glTexCoord2f(1 - (i+1) / (float)section, 1 - j / (float)slice);
			glNormal3f(cos(theta1)*cos(phi2), sin(theta1), cos(theta1)*sin(phi2));
			glVertex3f(cos(theta1)*cos(phi2), sin(theta1), cos(theta1)*sin(phi2));

			glTexCoord2f(1 - (i+1) / (float)section, 1 - (j+1) / (float)slice);
			glNormal3f(cos(theta2)*cos(phi2), sin(theta2), cos(theta2)*sin(phi2));
			glVertex3f(cos(theta2)*cos(phi2), sin(theta2), cos(theta2)*sin(phi2));

			glTexCoord2f(1 - i / (float)section, 1 - (j + 1) / (float)slice);
			glNormal3f(cos(theta2)*cos(phi1), sin(theta2), cos(theta2)*sin(phi1));
			glVertex3f(cos(theta2)*cos(phi1), sin(theta2), cos(theta2)*sin(phi1));
		}
	}

	glEnd();
	glPopMatrix();
}
//绘制圆柱体
void drawCylinder()
{
	glPushMatrix();

	int section = 50;//绘制精度

	glBegin(GL_POLYGON);//绘制一个多边形
    
    //绘制下底面
    //绘制50边形—逼近圆
	for (int i = 0; i < section; ++i)
	{
		float phi1 = radians(360.0f / section*i);
		glTexCoord2f(sin(phi1)*0.5 + 0.5, cos(phi1)*0.5 + 0.5);
		glNormal3f(0, -1, 0);
		glVertex3f(cos(phi1), -1, sin(phi1));
	}
	glEnd();

    //绘制上底面
	glBegin(GL_POLYGON);
	for (int i = 0; i < section; ++i)
	{
		float phi1 = radians(360.0f / section*i);
		glTexCoord2f(sin(phi1)*0.5 + 0.5, cos(phi1)*0.5 + 0.5);
		glNormal3f(0, 1, 0);
		glVertex3f(cos(phi1), 1, sin(phi1));
	}
	glEnd();

    //绘制量两底面之间连接部分
    //同球体类似，每次绘制一个小平面
	glBegin(GL_QUADS);
	for (int i = 0; i < section; ++i)
	{
		float phi1 = radians(360.0f / section*i), phi2 = radians(360.0f / section*(i + 1));

		glTexCoord2f(1 - i / (float)section, 1);
		glNormal3f(cos(phi1),0 , sin(phi1));
		glVertex3f(cos(phi1), -1, sin(phi1));

		glTexCoord2f(1 - (i + 1) / (float)section, 1);
		glNormal3f(cos(phi2), 0, sin(phi2));
		glVertex3f(cos(phi2),-1 , sin(phi2));

		glTexCoord2f(1 - (i + 1) / (float)section,0);
		glNormal3f(cos(phi2),0 , sin(phi2));
		glVertex3f(cos(phi2),1 , sin(phi2));

		glTexCoord2f(1 - i / (float)section, 0);
		glNormal3f(cos(phi1), 0, sin(phi1));
		glVertex3f(cos(phi1),1 , sin(phi1));
	}

	glEnd();
	glPopMatrix();
}

//绘制初始显示状态
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//清楚颜色缓冲以及深度缓冲
	glColor3f(1, 1, 1);//设置颜色为白色

	glMatrixMode(GL_MODELVIEW);//接下来模型视景操作
	glLoadIdentity();//恢复初始坐标系

    //gluLookAt:定义视图矩阵，与当前矩阵相乘，读入三组数据
    //第一组cameraPos.x, cameraPos.y, cameraPos.z,相机在世界坐标的位置
    //第二组(cameraPos + lookDir).x,c(cameraPos + lookDir).y,(cameraPos + lookDir).z
    //--相机镜头对准的物体在世界坐标的位置
    //第三组up.x,up,y,up,z 相机向上的方向在世界坐标中的方向
    gluLookAt(cameraPos.x, cameraPos.y, cameraPos.z, (cameraPos + lookDir).x, (cameraPos + lookDir).y, (cameraPos + lookDir).z, up.x, up.y, up.z);
	viewMat = lookAt(cameraPos, cameraPos + lookDir, up);//视图矩阵

	glLightfv(GL_LIGHT0, GL_POSITION, lightPos1);//设置光源的位置属性
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);//设置散射光

	//绘制底面
    glBindTexture(GL_TEXTURE_2D, floorTex);//读入底面纹理
	glPushMatrix();
	glScalef(10, 0, 10);//将x,z坐标各扩大十倍
	drawPlane();
	glPopMatrix();

	//绘制光源点
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    //保存之前的属性，确保可以返回原渲染路径（此处为纹理和灯光属性）
	glDisable(GL_TEXTURE_2D);//禁用纹理
	glDisable(GL_LIGHTING);//禁用灯光
	glColor3f(diffuse[0], diffuse[1], diffuse[2]);//设置新的颜色（变换光源颜色）
	glTranslatef(lightPos1[0], lightPos1[1], lightPos1[2]);
	glScalef(0.1, 0.1, 0.1);//缩放为原来0.1倍
	drawSphere();//将光源移动到其所在位置
	glEnable(GL_TEXTURE_2D);//启用纹理
	glEnable(GL_LIGHTING);//启用灯光
	glPopAttrib();//恢复属性
	glPopMatrix();

    //绘制立方体
	glPushMatrix();
	objectData[0].lighting ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);//检查光照
	glTranslatef(objectData[0].position.x, objectData[0].position.y, objectData[0].position.z);
    //移动到所在位置
    //操纵物体旋转，初始旋转角度为0
	glRotatef(objectData[0].rotateX, 0, 1, 0);
	glRotatef(objectData[0].rotateY, 1, 0, 0);
	glBindTexture(GL_TEXTURE_2D, boxTex);//绘制纹理
	drawCube();
	glPopMatrix();

	//绘制球体，相关操作与绘制立方体类似
    glPushMatrix();
	objectData[1].lighting ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	glTranslatef(objectData[1].position.x, objectData[1].position.y, objectData[1].position.z);
	glRotatef(objectData[1].rotateX, 0, 1, 0);
	glRotatef(objectData[1].rotateY, 1, 0, 0);
	glBindTexture(GL_TEXTURE_2D, earthTex);
	drawSphere();
	glPopMatrix();

    //绘制圆柱体，相关操作与绘制立方体类似
	glPushMatrix();
	objectData[2].lighting ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
	glTranslatef(objectData[2].position.x, objectData[2].position.y, objectData[2].position.z);
	glRotatef(objectData[2].rotateX, 0, 1, 0);
	glRotatef(objectData[2].rotateY, 1, 0, 0);
	glBindTexture(GL_TEXTURE_2D, pepsiTex);
	drawCylinder();
	glPopMatrix();
    
	glutSwapBuffers();
}

void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, w / float(h), 0.1, 100);

	projectionMat = perspective(radians(60.0f), w / float(h), 0.1f, 100.0f);
}

void spinDisplay(void)
{
	static int time = clock();
	int newT = clock();
	float dt = float(newT - time) / CLOCKS_PER_SEC;
	time = newT;

	float speed = 10.0;
	vec3 f = normalize(lookDir);
	vec3 u = normalize(up);
	vec3 r = cross(f, u);

	if (keyDown[0])
	{
		cameraPos += f*speed*dt;
	}
	if (keyDown[1])
	{
		cameraPos -= f*speed*dt;
	}
	if (keyDown[2])
	{
		cameraPos -= r*speed*dt;
	}
	if (keyDown[3])
	{
		cameraPos += r*speed*dt;
	}

	glutPostRedisplay();
}

void mouseMove(int x, int y)
{
	float dx = x - lastX, dy = y - lastY;
	lastX = x, lastY = y;

	if (cameraMoving)
	{
		float ratio = 0.1;
		vec3 r = normalize(cross(lookDir, up));
		up = normalize(cross(r, lookDir));

		mat4 rot = mat4();
		rot = rotate(mat4(), radians(-dx*ratio), vec3(0,1,0));
		lookDir = normalize(vec3(rot*vec4(lookDir, 0)));
		up = normalize((vec3(rot*vec4(up, 0))));
		r = normalize((vec3(rot*vec4(r, 0))));

		rot = rotate(mat4(), radians(-dy*ratio), r);
		lookDir = normalize((vec3(rot*vec4(lookDir, 0))));
		up = normalize((vec3(rot*vec4(up, 0))));

	}
	else if (selectedIndex>=0)
	{
		if (!objectRotating)
		{
			if (selectedIndex >= 0 && selectedIndex < 3)
			{
				vec4 lastPos = projectionMat*viewMat*vec4(objectData[selectedIndex].position, 1);
				float w = lastPos.w;
				lastPos /= lastPos.w;
				lastPos = lastPos*0.5f + 0.5f;

				vec3 destPos = vec3(float(x) / glutGet(GLUT_WINDOW_WIDTH), 1 - float(y) / glutGet(GLUT_WINDOW_HEIGHT), lastPos.z);
				destPos = destPos*2.0f - 1.0f;
				vec4 p = inverse(projectionMat*viewMat)*vec4(destPos*w, w);

				objectData[selectedIndex].position = vec3(p);
			}
			else
			{
				vec4 lastPos = projectionMat*viewMat*vec4(lightPos1[0], lightPos1[1], lightPos1[2], 1);
				float w = lastPos.w;
				lastPos /= lastPos.w;
				lastPos = lastPos*0.5f + 0.5f;

				vec3 destPos = vec3(float(x) / glutGet(GLUT_WINDOW_WIDTH), 1 - float(y) / glutGet(GLUT_WINDOW_HEIGHT), lastPos.z);
				destPos = destPos*2.0f - 1.0f;
				vec4 p = inverse(projectionMat*viewMat)*vec4(destPos*w, w);

				lightPos1[0] = p.x;
				lightPos1[1] = p.y;
				lightPos1[2] = p.z;
			}
		}
		else
		{
			if (selectedIndex >= 0 && selectedIndex < 3)
			{
				objectData[selectedIndex].rotateX += dx;
				objectData[selectedIndex].rotateY += dy;
			}
		}
	}
}

void mousePress(int button, int state, int x, int y)
{
	lastX = x;
	lastY = y;
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
	{
		if (state==GLUT_DOWN)
		{
			selectedIndex = -1;
			vec2 windowSize = { glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT) };
			float minDis = 1000;
			for (int i = 0; i < 3; ++i)
			{
				vec4 NDC = projectionMat*viewMat*vec4(objectData[i].position, 1);
				NDC /= NDC.w;
				NDC = NDC*0.5f + 0.5f;
				vec2 pos = vec2{ NDC.x,1 - NDC.y }*windowSize;
				float dis = length(pos - vec2(x, y));
				if (minDis > dis)
				{
					minDis = dis;
					if (minDis < 20)
					{
						selectedIndex = i;
					}
				}
			}

			vec4 NDC = projectionMat*viewMat*vec4(lightPos1[0], lightPos1[1], lightPos1[2], 1);
			NDC /= NDC.w;
			NDC = NDC*0.5f + 0.5f;
			vec2 pos = vec2{ NDC.x,1 - NDC.y }*windowSize;
			float dis = length(pos - vec2(x, y));
			if (minDis > dis)
			{
				minDis = dis;
				if (minDis < 20)
				{
					selectedIndex = 3;
				}
			}
			printf("selected:%d\n", selectedIndex);
		}
	}
	break;
	case GLUT_RIGHT_BUTTON:
	{
		selectedIndex = -1;
		cameraMoving = state == GLUT_DOWN;
	}
	break;
	default:
		break;
	}
}

//判定键盘是否读入key值
//有效读入值只有‘w','a','s','d','r','c'
void keyboardPress(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		keyDown[0] = true;
		break;
	case 's':
		keyDown[1] = true;
		break;
	case 'a':
		keyDown[2] = true;
		break;
	case 'd':
		keyDown[3] = true;
		break;
	case 'r':
		objectRotating = true;
		break;
	case 'c':  //生成一组随机RGB值，改变光源颜色
		diffuse[0] = rand() % 128 / 255.0 + 0.5;
		diffuse[1] = rand() % 128 / 255.0 + 0.5;
		diffuse[2] = rand() % 128 / 255.0 + 0.5;
		break;
	default:
		break;
	}
}

//判定键盘是否松开
void keyboardUp(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		keyDown[0] = false;
		break;
	case 's':
		keyDown[1] = false;
		break;
	case 'a':
		keyDown[2] = false;
		break;
	case 'd':
		keyDown[3] = false;
		break;
	case 'r':
		objectRotating = false;
		break;
	default:
		break;
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);//初始化glut库
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);//双缓存，RGB颜色模式
	glutInitWindowSize(500, 500);//创建窗口
	glutInitWindowPosition(100, 100);//初始化窗口位置
    glutCreateWindow("Graphics Pipeline");//设置窗口标题
	glutDisplayFunc(display);//绘图函数
	glutReshapeFunc(reshape);//窗口改变调用函数
	glutMotionFunc(mouseMove);//设置鼠标移动响应函数
	glutMouseFunc(mousePress);//设置鼠标点击响应函数
	glutIdleFunc(spinDisplay);//设置全局回调函数，识别窗口事件
	glutKeyboardFunc(keyboardPress);//设置键盘响应函数
	glutKeyboardUpFunc(keyboardUp);//判定键盘键松开

	glEnable(GL_NORMALIZE);//启用法向量
	glEnable(GL_LIGHTING);//启用灯光源
	glEnable(GL_LIGHT0);//启用0号灯光源

	glClearColor(0.3, 0.3, 0.3, 1.0);//设置刷新时候的颜色值

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    //设置光的环境强度，默认值(0, 0, 0, 1)（不存在）
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    //设置散色光，GL_LIGHT0默认为（1, 1, 1, 1）、GL_LIGHT1-7默认为（0, 0, 0, 1）
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    //设置镜面光，GL_LIGHT0默认为（1, 1, 1, 1）、GL_LIGHT1-7默认为（0, 0, 0, 1 ）

	glEnable(GL_DEPTH_TEST);
    //启用后，OpenGL在绘制的时候就会检查，当前像素前面是否有别的像素，只绘制最前面一层

	glDisable(GL_CULL_FACE);//关闭面剔除
	glEnable(GL_TEXTURE_2D);//开启纹理功能（二维纹理）

	boxTex = loadTexture("/usr/local/include/container.png");//设置立方体纹理
	floorTex = loadTexture("/usr/local/include/floor.jpg");//设置底板纹理
	earthTex = loadTexture("/usr/local/include/earth.jpg");//设置球体纹理
	pepsiTex= loadTexture("/usr/local/include/pepsi.jpg");//设置圆柱体纹理
	glutMainLoop();//开始主循环
	return 0;
}
