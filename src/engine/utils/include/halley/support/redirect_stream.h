/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <iostream>
#include <memory>

namespace Halley {
	class RedirectStream : public std::streambuf {
	public:
		RedirectStream(std::ostream& source, bool exclusive);
		virtual ~RedirectStream();

		RedirectStream& operator=(const RedirectStream&) = delete;

	protected:
		std::streamsize xsputn(const char *msg, std::streamsize count) override;
		int sync() override { return 0; }
		int overflow(int c) override;

		virtual void onText(const char* /*msg*/, std::streamsize /*count*/) {}
		virtual void onFlush() {}

	private:
		std::streambuf* orgbuf;
		std::ostream& orgstream;
		bool exclusive;
		std::shared_ptr<std::ostream> newstream;
	};

	class RedirectStreamToStream final : public RedirectStream {
	public:
		RedirectStreamToStream(std::ostream& source, std::shared_ptr<std::ostream> dst, bool exclusive);
		~RedirectStreamToStream();

	protected:
		void onText(const char *msg, std::streamsize count) override;
		void onFlush() override;

		std::shared_ptr<std::ostream> dst;
	};
}
