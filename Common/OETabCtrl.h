/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once
#ifndef __OETABCTRL_H__
#define __OETABCTRL_H__

#include "afxcmn.h"

class COETabCtrl :
	public CTabCtrl
{
public:
	COETabCtrl(void);
	~COETabCtrl(void);

protected:
	//{{AFX_MSG(COETabCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
//	afx_msg void OnPaint();
//	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
	DECLARE_MESSAGE_MAP()

	bool m_button_is_down;
	int m_first_selected_item;

    bool m_highlighting_for_dragged;                // highlight an item when it's dragged
    bool m_highlighting_threshold_exceeded;         // prevent highlighting on mouse click
    CPoint m_button_down_point, m_last_move_point;  // data for implementation of highlighting threshold
    enum { SD_NONE, SD_RIGHT, SD_LEFT } m_scrolling;// directon of autoscrolling

    static const int HIGHLIGHTING_THRESHOLD_CX = 2;
    static const int HIGHLIGHTING_THRESHOLD_CY = 4;
    static const int SCROLLING_THRESHOLD_CX    = 16;
    static const UINT DRAG_MOUSE_TIMER         = (UINT)-1;
    static const UINT DRAG_MOUSE_TIMER_ELAPSE  = 200;

	HCURSOR g_hCurArrow;
	HCURSOR g_hCurNo;
};
#endif //__OETABCTRL_H__