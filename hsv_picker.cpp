#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

std::vector<cv::Vec3b> pixels_read;
cv::Mat input;


bool connect(boost::asio::ip::tcp::socket & socket, boost::asio::ip::tcp::endpoint & endpoint) {
		try{
      std::cout << "Connect to " << endpoint << " " << std::flush;
			socket.connect(endpoint);
      std::cout << "Connected." << std::endl;
      return true;
		}
		catch(std::exception & e){
			std::cout << "Failed, trying to reconnect." << std::endl;
			return false;
		}
}

void mouse_callback(int event, int x, int y, int flags, void* userdata)
{
  if ( event == cv::EVENT_LBUTTONDOWN ) {
    pixels_read.push_back( input.at<cv::Vec3b>(y,x) );
  }
  else if (event == cv::EVENT_RBUTTONDOWN) {
    pixels_read.clear();
  }
}

cv::Vec3b min_hsv() {
  cv::Vec3b min = cv::Vec3b::all(255);
  for (auto & p : pixels_read) {
    if (p[0] < min[0]) {
      min[0] = p[0];
    }

    if (p[1] < min[1]) {
      min[1] = p[1];
    }

    if (p[2] < min[2]) {
      min[2] = p[2];
    }
  }
  return min;
}

cv::Vec3b max_hsv() {
  cv::Vec3b max = cv::Vec3b::all(0);
  for (auto & p : pixels_read) {
    if (p[0] > max[0]) {
      max[0] = p[0];
    }

    if (p[1] > max[1]) {
      max[1] = p[1];
    }

    if (p[2] > max[2]) {
      max[2] = p[2];
    }
  }
  return max;
}


int main(int argc, char *argv[]) {

  /*if(argc == 2) {
    if ( !boost::filesystem::exists(argv[2]) ) {
      std::cout << "File " << argv[2] << " does not exist!" << std::endl;
      return 1;
    }
    std::cout << "Using " << argv[2] << " to pick the hsv range." << std::endl;
    input = cv::imread(argv[2]);
  }*/

  if(argc < 2){
		std::cout << "Es muss die IP-Adresse des Pi Servers angegeben werden!" << std::endl;
		std::cout << "Aufruf: ./hsv_picker IP_ADRESSE [window name]" << std::endl;
		return -1;
	}
	std::string pick_window_name = "input image";
	if(argc == 3) {
		pick_window_name = std::string(argv[2]);
	}

  boost::asio::io_service ios;
  boost::asio::ip::tcp::endpoint endpoint;

  // fetch ip address
  try {
    endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(argv[1]), 1300);
  }
  catch(std::exception & e){
    std::cerr << e.what() << std::endl;
    std::cout << "IP-Adresse nicht korrekt übergeben?" << std::endl;
    return -1;
  }

  // socket to pi server
  boost::asio::ip::tcp::socket socket(ios);

  while(!connect(socket, endpoint)) {
    std::this_thread::sleep_for (std::chrono::seconds(5));
  }

  cv::namedWindow("Image");
  cv::setMouseCallback("Image", mouse_callback, NULL);

  int key = 0;
  do {

    try{
			uint32_t sz;
			boost::asio::read(socket, boost::asio::buffer(&sz, sizeof(sz)));

			std::vector<char> nameBuf;
			nameBuf.resize(20);
			boost::asio::read(socket, boost::asio::buffer(nameBuf, 20));

			std::string name(nameBuf.data(), std::strlen(nameBuf.data()));

			//std::cout << "Window: " << name << std::endl;

			std::vector<uchar> img_buf;
			img_buf.resize(sz);
			boost::asio::read(socket, boost::asio::buffer(img_buf.data(), sz));

      cv::Mat reveived_image = cv::imdecode(img_buf, 1);

			if(name == pick_window_name) {
				if(name != "hsv") {
					cv::cvtColor(reveived_image, input, cv::COLOR_RGB2HSV);
				} else {
					input = reveived_image;
				}

        std::stringstream num_pixels, min_max;
        num_pixels << "Pixels analyzed: " << pixels_read.size();
        min_max << "min/max: " << min_hsv() << "/" << max_hsv();

        putText(input, num_pixels.str(), cv::Point(20,40) , cv::FONT_HERSHEY_SIMPLEX, .7, cv::Scalar(0,255,0), 2,8,false);
        if(pixels_read.size() > 0) {
          putText(input, min_max.str(), cv::Point(20,80) , cv::FONT_HERSHEY_SIMPLEX, .7, cv::Scalar(0,255,0), 2,8,false);
        }

        cv::imshow("Image", input);

			}
		}
		catch(std::exception & e){
			std::cerr << e.what() << std::endl;
			std::cout << "Connection broken. Trying to reconnect" << std::endl;
			socket.close();
			cv::destroyAllWindows();

      while(!connect(socket, endpoint)) {
        std::this_thread::sleep_for (std::chrono::seconds(1));
      }
		}

    key = cv::waitKey(20);
  } while (key != 27);



  return 0;
}
