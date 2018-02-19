#ifndef POL_BUTTON_SET_CONTROLLER_H
#define POL_BUTTON_SET_CONTROLLER_H

class PolButtonSetController
{
public:
	PolButtonSetController() :
		_ppSelected(true),
		_pqSelected(false),
		_qpSelected(false),
		_qqSelected(true)
	{ }
	
	bool IsPPSelected() const { return _ppSelected; }
	bool IsPQSelected() const { return _pqSelected; }
	bool IsQPSelected() const { return _qpSelected; }
	bool IsQQSelected() const { return _qqSelected; }
private:
	bool _ppSelected, _pqSelected, _qpSelected, _qqSelected;
};

#endif
