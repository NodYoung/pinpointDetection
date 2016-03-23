#include <opencv2/core/core.hpp>  
#include <opencv2/imgproc/imgproc.hpp> 
#include <opencv2/highgui/highgui.hpp>  
#include <iostream>  
#include <map>
using namespace std;  
using namespace cv;  

bool intersection(Point2f o1, Point2f p1, Point2f o2, Point2f p2,Point2f &r);
double lengthLine(Point a, Point b);
float angle(Point2f A, Point2f B); 

typedef multimap<float, Vec4i> mapline;

int main()
{
	//载入灰度图像
	Mat srcImage = imread("donut.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	imshow("【原始图】",srcImage);//显示载入的图片
	//二值化
	Mat biImage;
	threshold(srcImage, biImage, 200, 255, CV_THRESH_BINARY_INV ); //对灰度图进行二值化处理,前景变为白色后findContours()寻找外轮廓才好用
	imshow("【二值化图】",biImage);
	//去除图像上下左右的边框
	Size sizeImage = Size(700, 475);
	Mat imageROI = biImage(Rect(10, 25, sizeImage.width, sizeImage.height));
	Mat removeFrameImage = imageROI.clone();
	imshow("【去除图像边框】",removeFrameImage);
	//将图像最外层的一圈像素填充为黑色
	rectangle(removeFrameImage, Point(0, 0), Point(699, 474), Scalar(0, 0, 0));
	imshow("【去除图像边框】",removeFrameImage);
	//查找最外层轮廓
	Mat contourImage(removeFrameImage.rows, removeFrameImage.cols, CV_8UC1, Scalar(0, 0, 0));
	Mat pipetteImage(removeFrameImage.rows, removeFrameImage.cols, CV_8UC1, Scalar(0, 0, 0));
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( removeFrameImage, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	Mat pipetteROI;
	for (int i = 0; i < contours.size(); i++)
	{
		drawContours( contourImage, contours, i, Scalar(255, 255, 255), 1, 8, hierarchy, 1);//绘制轮廓
		Rect rect = boundingRect(Mat(contours[i]));
		if (rect.width/rect.height > 2)
		{
			drawContours( pipetteImage, contours, i, Scalar(255, 255, 255), 1, 8, hierarchy, 1);//绘制轮廓
		}
	}
	imshow( "轮廓图", contourImage);
	imshow("micropipette", pipetteImage);
	//对针尖进行膨胀操作
	Mat dilateImage;
	Mat element = getStructuringElement(MORPH_RECT, Size(2*1+1, 2*1+1),Point( 1, 1 ));
	dilate(pipetteImage, dilateImage, element);
	imshow("膨胀图", dilateImage);
	//直线检测找到针尖位置
	Mat lineImage(dilateImage.rows, dilateImage.cols, CV_8UC1, Scalar(0, 0, 0));
	vector<Vec4i> lines;//定义一个矢量结构lines用于存放得到的线段矢量集合
	HoughLinesP(dilateImage, lines, 1, CV_PI/180, 200, 50, 10 );
	mapline mp;//将检测到的直线及其斜率存入map类当中
	//依次在图中绘制出每条线段
	for( size_t i = 0; i < lines.size(); i++ )
	{
		Vec4i l = lines[i];
		line( lineImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,255,255), 1, CV_AA);
		cout << "Line[(" << l[0] << "," << l[1] <<"),("  << l[2] << "," << l[3] << ")]: ";
		cout << lengthLine(Point(l[0], l[1]), Point(l[2], l[3]));
		cout << "; " << angle(Point(l[0], l[1]), Point(l[2], l[3])) << endl;
		mp.insert(mapline::value_type(angle(Point(l[0], l[1]), Point(l[2], l[3])), l));
	}
	imshow( "直线图", lineImage);
	//找出斜率最小和最大的两条线的交点
	Vec4i upline = mp.begin()->second;
	Vec4i downline = (--mp.end())->second;
	Point o1 = Point(upline[0], upline[1]);
	Point p1 = Point(upline[2], upline[3]);
	Point o2 = Point(downline[0], downline[1]);
	Point p2 = Point(downline[2], downline[3]);
	Mat pointImage;
	cvtColor(contourImage.clone(), pointImage, CV_GRAY2RGB);//RGB图像的灰度化
	Point2f insection;
	if (intersection(o1, p1, o2, p2, insection) == true)
	{
		circle( pointImage, insection, 2, Scalar(0,255,0), 1, CV_AA);
	}
	imshow( "交点图", pointImage);
	imwrite("result.jpg",pointImage);//保存图片



	/*
	//查看图片数据
	ofstream outImage("imagedata.txt", ios::out | ios::binary);    
	for( unsigned int nrow = 0; nrow < removeFrameImage.rows; nrow++)  
	{  
		for(unsigned int ncol = 0; ncol < removeFrameImage.cols; ncol++)  
		{  
			uchar val = removeFrameImage.at<unsigned char>(nrow,ncol);    
			outImage << (int(val) > 200 ? 1 :0) ; //File3<<int(val)<< endl ;
		}   
		outImage << endl ;  
	}  
	outImage.close(); 
*/

	//等待任意按键按下
	waitKey(0);
	return 0;
}

// This function calculates the angle of the line from A to B with respect to the positive X-axis in degrees
float angle(Point2f A, Point2f B) 
{
	float val = (B.y-A.y)/(B.x-A.x); // calculate slope between the two points
	val = val - pow(val,3)/3 + pow(val,5)/5; // find arc tan of the slope using taylor series approximation
	val = ((int)(val*180/3.14)) % 360; // Convert the angle in radians to degrees
	if(B.x < A.x) val+=180;
	if(val < 0) val = 360 + val;
	return val;
}

double lengthLine(Point a, Point b)
{
	double res = cv::norm(a-b);//Euclidian distance
	return res;
}

// Finds the intersection of two lines, or returns false.
// The lines are defined by (o1, p1) and (o2, p2).
bool intersection(Point2f o1, Point2f p1, Point2f o2, Point2f p2,Point2f &r)
{
	Point2f x = o2 - o1;
	Point2f d1 = p1 - o1;
	Point2f d2 = p2 - o2;

	float cross = d1.x*d2.y - d1.y*d2.x;
	if (abs(cross) < /*EPS*/1e-8)
		return false;

	double t1 = (x.x * d2.y - x.y * d2.x)/cross;
	r = o1 + d1 * t1;
	return true;
}