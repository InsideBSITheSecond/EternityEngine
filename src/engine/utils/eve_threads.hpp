#pragma once

#include "../game/eve_terrain.hpp"

#include <boost/chrono.hpp>
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
			std::size_t MAX_THREADS = 1;

			void faketask(std::string msg){
				boost::this_thread::sleep_for(boost::chrono::milliseconds(rand() % 3000 + 1000));
				std::cout << msg << std::endl;
			}

			EveThreadPool(std::size_t threadCount) : MAX_THREADS{threadCount} {
				/*
				* This will start the io_service_ processing loop. All tasks
				* assigned with io_service_->post() will start executing.
				*/
				io_service_ = boost::make_shared<boost::asio::io_service>();
				work_ = boost::make_shared<boost::asio::io_service::work>(*io_service_);

				std::cout << "created thread pool of size" << MAX_THREADS << std::endl;
				for (std::size_t i = 0; i < MAX_THREADS; ++i){
					threadpool_.create_thread(boost::bind(&boost::asio::io_service::run, io_service_));
				}
			}

			~EveThreadPool() {
				destroy();
			}

			void pushChunkToRemeshingQueue(Chunk *chunk) {
				io_service_->post(boost::bind(&Chunk::remesh, chunk, chunk->root));
			}

			void runFakeTasks(std::size_t jobsize) {
				std::cout << "adding " << std::to_string(jobsize) << " jobs to the job pool" << std::endl;
				for (std::size_t i = 0; i < jobsize; ++i){
					io_service_->post(boost::bind(&EveThreadPool::faketask, this, "fake task " + std::to_string(i) + " ended"));
				}
			}

			void destroy() {
				io_service_->stop();
				threadpool_.join_all();
			}

		private:
			boost::shared_ptr<boost::asio::io_service> io_service_;
			boost::shared_ptr<boost::asio::io_service::work> work_;
			boost::thread_group threadpool_;
	};
}