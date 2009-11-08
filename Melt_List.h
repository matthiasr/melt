/*
	--------------------------------------------------------------------------------
	
		Melt - A GUI Frontend for CDR Tools
		©2000 Lukas Hartmann / Atomatrix
	
	--------------------------------------------------------------------------------
	                  Atomatrix Open Source License v1.0 [AOSL1]
	--------------------------------------------------------------------------------
	
	Terms and Conditions
	Copyright 2000, Lukas Hartmann of Atomatrix. All rights reserved.
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software with limited restriction, including the rights to use, copy, modify,
	merge, publish, distribute, sublicense, and/or sell copies of the Software as
	long as there is no profit made by doing so, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice applies to all licensees
	and shall be included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	LUKAS HARTMANN BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
	AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
	--------------------------------------------------------------------------------
	
	Your Contact: Lukas Hartmann, atomatrix@gmx.de
				  http://www.atomatrix.com
	
	Please be aware that some parts of this code are really shitty, because I'm
	really lazy sometimes ;)
	
	So Long, And Thanks For All The Fish!
*/

class MeltList : public BListView
{
	public:
		MeltList();
		
		void RePattern();
		virtual void MessageReceived(BMessage* msg);
		virtual void MouseDown(BPoint where);
		
		BPopUpMenu *TrackPop;
		
	private:
		int32 mLastButton;
		int32 mClickCount;
};

class MeltItem : public BListItem
{
	public:
		MeltItem(char* applyfile,char* applyname,const BBitmap* applyicon,const BBitmap* applyicon1,const BBitmap* applyicon2);
		virtual void DrawItem(BView *owner, BRect frame, bool complete = false);
	
		char fname[B_FILE_NAME_LENGTH+500];
		bool Pattern;
		
	private:
		int type;
		char name[B_FILE_NAME_LENGTH+50];
		
		const BBitmap* icon;
		const BBitmap* icon_p;
		const BBitmap* icon_s;
};