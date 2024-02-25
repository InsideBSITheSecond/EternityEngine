#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

namespace eve {
	class EveThreadPool {
		public:

			void callback(std::string msg){
				std::cout << msg << std::endl;
			}

			EveThreadPool() {
				/*
				* This will start the io_service_ processing loop. All tasks
				* assigned with io_service_->post() will start executing.
				*/
				io_service_ = boost::make_shared<boost::asio::io_service>();
				work_ = boost::make_shared<boost::asio::io_service::work>(*io_service_);

				/*
				* This will add 2 threads to the thread pool. (You could just put it in a for loop)
				*/
				std::size_t my_thread_count = 2;
				for (std::size_t i = 0; i < my_thread_count; ++i){
					threadpool_.create_thread(boost::bind(&boost::asio::io_service::run, io_service_));
				}
			}

			~EveThreadPool() {
				stop();
			}

			void run() {
				io_service_->post(boost::bind(&EveThreadPool::callback,this, "Hello World!"));
				io_service_->post(boost::bind(&EveThreadPool::callback,this, "./cache"));
				io_service_->post(boost::bind(&EveThreadPool::callback,this, "twitter,gmail,facebook,tumblr,reddit"));
			}

			void stop() {
				io_service_->stop();
				threadpool_.join_all();
			}

		private:
			boost::shared_ptr<boost::asio::io_service> io_service_;
			boost::shared_ptr<boost::asio::io_service::work> work_;
			boost::thread_group threadpool_;
	};
}