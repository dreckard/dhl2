#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
	class ImagePanel;
}

class CDHL_Panel
{
public:
	virtual void	Create( vgui::VPANEL parent ) = 0;
	virtual void	Destroy( void ) = 0;
};

extern CDHL_Panel *dhlPanel;