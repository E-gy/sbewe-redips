#pragma once

namespace redips::ssl {

class SSLStateControl {
	public:
		SSLStateControl();
		~SSLStateControl();
		SSLStateControl(const SSLStateControl&) = delete;
		SSLStateControl(SSLStateControl&&) = delete;
};

}
