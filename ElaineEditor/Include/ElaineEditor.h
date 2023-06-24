#pragma once

namespace Elaine
{
	class ElaineEditor
	{
	public:
		ElaineEditor();
		virtual ~ElaineEditor();
		void			initialize();
		void			close();
		void			tick(float dt);
	private:

	};
}