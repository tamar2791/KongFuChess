#include "OpenCvImg.hpp"


#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <vector>
#include <functional>

// פונקציה גלובלית לטיפול במקשים
std::function<void(int)> global_key_handler = nullptr;

struct OpenCvImg::Impl {
	cv::Mat mat;
};

OpenCvImg::OpenCvImg() : impl(std::make_unique<Impl>()) {}
OpenCvImg::~OpenCvImg() = default;
ImgPtr OpenCvImg::clone() const
{
	auto res = std::make_shared<OpenCvImg>();
	res->impl->mat = this->impl->mat.clone();
	return res;
}

void OpenCvImg::read(const std::string& path, const std::pair<int, int>& size) {
	impl->mat = cv::imread(path, cv::IMREAD_UNCHANGED);
	if (impl->mat.empty()) throw std::runtime_error("Cannot load image: " + path);
	if (size.first > 0 && size.second > 0) {
		cv::resize(impl->mat, impl->mat, cv::Size(size.first, size.second));
	}
}

std::pair<int,int> OpenCvImg::size() const {
	return {impl->mat.cols, impl->mat.rows};
}

void OpenCvImg::create_blank(int w, int h) {
	impl->mat = cv::Mat(h, w, CV_8UC4, cv::Scalar(0, 0, 0, 0));
}

void OpenCvImg::draw_on(Img& dst, int x, int y) {
	auto* cvDst = dynamic_cast<OpenCvImg*>(&dst);
	if (!cvDst) return;
	if (impl->mat.empty()) return;
	
	// בדיקת גבולות
	if (x < 0 || y < 0 || x + impl->mat.cols > cvDst->impl->mat.cols || y + impl->mat.rows > cvDst->impl->mat.rows) {
		return;
	}
	
	cv::Mat src = impl->mat;
	cv::Mat& dstMat = cvDst->impl->mat;
	
	// וידוא שלשתי התמונות יש אותו מספר ערוצים
	if (src.channels() != dstMat.channels()) {
		if (src.channels() == 3 && dstMat.channels() == 4) {
			cv::cvtColor(src, src, cv::COLOR_BGR2BGRA);
		} else if (src.channels() == 4 && dstMat.channels() == 3) {
			cv::cvtColor(src, src, cv::COLOR_BGRA2BGR);
		}
	}
	
	src.copyTo(dstMat(cv::Rect(x, y, src.cols, src.rows)));
}

void OpenCvImg::put_text(const std::string& txt, int x, int y, double font_size) {
	if (impl->mat.empty()) return;
	cv::putText(impl->mat, txt, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX, font_size, cv::Scalar(255, 255, 255, 255));
}

void OpenCvImg::show() const {
    if (impl->mat.empty()) {
        std::cout << "[DEBUG] OpenCvImg::show() - mat is empty!" << std::endl;
        return;
    }
    
    static bool first_show = true;
    if (first_show) {
        std::cout << "[DEBUG] First time showing window - size: " << impl->mat.cols << "x" << impl->mat.rows << std::endl;
        cv::namedWindow("KFC Game - Click here and use keyboard!", cv::WINDOW_AUTOSIZE);
        first_show = false;
    }
    
    cv::imshow("KFC Game - Click here and use keyboard!", impl->mat);
    
    int key = cv::waitKey(1);
    if (key != -1 && key != 255) {
        std::cout << "[DEBUG] Key detected: " << key << std::endl;
        if (global_key_handler) {
            global_key_handler(key);
        }
    }
}
void OpenCvImg::draw_rect(int x, int y, int width, int height, const std::vector<uint8_t> & color) {
	if (impl->mat.empty()) return;
	cv::Scalar cvColor = color.size() == 3 ? cv::Scalar(color[0], color[1], color[2]) : cv::Scalar(color[0], color[1], color[2], color[3]);
	cv::rectangle(impl->mat, cv::Rect(x, y, width, height), cvColor, 2);
}

// פונקציה להגדרת handler למקשים
void set_global_key_handler(std::function<void(int)> handler) {
	global_key_handler = handler;
}