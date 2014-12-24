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

/*
	fixed binary paths for Haiku. Note that THIS BREAKS BeOS R5 COMPATIBILITY
		-- 2009-11-08 Matthias Rampke
*/

#define MELT_SIG "application/x-vnd.atomatrix-melt"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <AppKit.h>
#include <InterfaceKit.h>
#include <StorageKit.h>

#include "StyleUtils.h"

#include "Melt_List.h"
#include "Melt_Burn.h"
#include "Melt_NewProject.h"
#include "Melt_VirtualCD.h"
#include "Melt_App.h"

int scrw=1024;
int scrh=768;
int winw=400;
int winh=400;

char Possible_Devices[20*7];

bool DO_LOG=false;
bool MELT_MULTISESSION=false;
int MELT_CDTYPE=0;                    // 0: ISO Data, 1: Audio
char* MELT_FIX="";
char* MELT_DUMMY="";
char* MELT_DEVICE;
char* MELT_PATH;
char MELT_RELOCATE[2048];
int MELT_SPEED=1;
int MELT_CDSIZE=0;

#define BLANK_MODE_MAX 4
char* BLANK_MODE[BLANK_MODE_MAX]={"Full","Fast (TOC,TMA,pregap)","Blank last session","Unclose last session"};
char* BLANK_CMD[BLANK_MODE_MAX]={"all","fast","session","unclose"};

int MELT_BLANKMODE=0;

bool is_saved=false;
bool Global_Pattern=false;

// Debugging -----------------------------------

void Log(char* text)
{
	if (!DO_LOG) return;
	FILE* logfile=fopen("/boot/MELT.LOG","a");
	fwrite(text,strlen(text),1,logfile);
	fwrite("\n",1,1,logfile);
	fclose(logfile);
};

// MeltNewProj ----------------------------------------------------------------------------------------------

FunkyLabel::FunkyLabel(BRect size,char* applylabel,char* gfx) : BView(size,"funkylabel",B_FOLLOW_NONE,B_WILL_DRAW)
{
	bitmap=FetchStyleResource(gfx);
	strcpy(label,applylabel);
};

FunkyLabel::~FunkyLabel()
{
	delete bitmap;
};

void FunkyLabel::Draw(BRect dummy)
{
	DrawBitmap(bitmap,BPoint(1,0));
	SetFont(be_bold_font);
	DrawString(label,BPoint(35,16));
};

MeltNewProj::MeltNewProj() : BWindow(BRect(scrw/2-winw/2,scrh/2-winh/2,scrw/2+winw/2,scrh/2+winh/2),
									 "Melt: New CD-R",B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	AroundBox=new BBox(BRect(0,0,winw,winh),"BetterStyle",B_FOLLOW_NONE,B_WILL_DRAW|B_FRAME_EVENTS,B_PLAIN_BORDER);

	LogBox=new BBox(BRect(3,3,winw-3,180),"LogBox",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	LogBox->SetLabel("CDRecord Log:");

	CDTypeBox=new BBox(BRect(3,181,winw-3,winh-73),"CDTypeBox",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	CTLabel=new FunkyLabel(BRect(0,0,35+be_bold_font->StringWidth("Mode Options:"),23),"Mode Options:","label_1");
	rgb_color grey={216,216,216};
	CTLabel->SetViewColor(grey);
	CDTypeBox->SetLabel(CTLabel);

	OptionBox=new BBox(BRect(3,winh-72,winw-3,winh-3),"OptionBox",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	OptionBox->SetLabel("Virtual CD Project:");
	ProjectLabel=new FunkyLabel(BRect(0,0,35+be_bold_font->StringWidth("Virtual CD Project:"),27),"Virtual CD Project:","label_4");
	ProjectLabel->SetViewColor(grey);
	OptionBox->SetLabel(ProjectLabel);

	LogView=new BTextView(BRect(8,18,winw-(15+B_V_SCROLL_BAR_WIDTH),winh-260),"cdrecord",BRect(5,15,winw-10,winh-160),B_FOLLOW_NONE);
	LogView->SetStylable(true);
	LogView->MakeEditable(false);

	LogScroll=new BScrollView("logscroll",LogView,B_FOLLOW_NONE,0,false,true,B_FANCY_BORDER);

	Recorders=new BMenu("Select");
	Recorders->SetLabelFromMarked(true);
	RecordPop=new BMenuField(BRect(5,winh-255,winw-10,winh-225),"recorder","Devices:",Recorders);
	RecordPop->SetDivider(be_plain_font->StringWidth("Devices: "));
	LogBox->AddChild(LogScroll);
	LogBox->AddChild(RecordPop);
	
	UseISO9660=new BRadioButton(BRect(160+34,40,180+78,60),"iso","ISO9660", new BMessage('uiso'));
	UseISO9660->SetValue(B_CONTROL_ON);
	UseBFS=new BRadioButton(BRect(200,32,winw-10,47),"bfs","BFS (Melt 1.5)", new BMessage('ubfs'));
	UseAudio=new BRadioButton(BRect(180+114,40,winw-10,60),"bfs","Audio", new BMessage('uaud'));
	
	DataIcon=new FunkyLabel(BRect(160,33,160+32,33+32),"","label_2");
	DataIcon->SetViewColor(grey);
	AudioIcon=new FunkyLabel(BRect(180+80,30,180+112,30+32),"","label_3");
	AudioIcon->SetViewColor(grey);
	
	MultiSession=new BCheckBox(BRect(5,25,159,40),"data2","Multisession", new BMessage('mses'));
	CheckDAO=new BCheckBox(BRect(5,49,100,64),"dao","Disk At Once",new BMessage('cdao'));
	CheckDAO->SetEnabled(false);
	//Audio=new BRadioButton(BRect(5,49,100,64),"audio","Audio", new BMessage('audi'));
	CDTypeBox->AddChild(UseISO9660);
	//CDTypeBox->AddChild(UseBFS);
	//CDTypeBox->AddChild(SingleSession);
	CDTypeBox->AddChild(MultiSession);
	CDTypeBox->AddChild(CheckDAO);
	CDTypeBox->AddChild(UseAudio);
	CDTypeBox->AddChild(DataIcon);
	CDTypeBox->AddChild(AudioIcon);

	CDRWBox=new BBox(BRect(5,75,winw-11,140),"CDRWBox",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	CDRWBox->SetLabel("CDRW Tools:");
	
	Modes=new BMenu("Select");
	Modes->SetLabelFromMarked(true);

	for (uint8 i=0; i<BLANK_MODE_MAX; i++)
		Modes->AddItem(new BMenuItem(BLANK_MODE[i],new BMessage('blk\0'|i)));

	ModePop=new BMenuField(BRect(5,15,230,40),"blankmode","Blank Mode:",Modes);
	ModePop->SetDivider(be_plain_font->StringWidth("Blank Mode: "));
	Blank=new BButton(BRect(300,22,winw-22,47),"blank","Blank",new BMessage('blnk'));
	
	BlankStatus=new BStatusBar(BRect(5,28,30,60),"status","","");
	
	BlankTxt=new BTextView(BRect(34,44,230,60),"txt",BRect(0,0,winw-52,15),B_FOLLOW_NONE);
	BlankTxt->MakeSelectable(false);
	BlankTxt->MakeEditable(false);
	BlankTxt->SetStylable(true);
	BlankTxt->SetViewColor(grey);
	BlankTxt->Insert("Waiting.");
	
	Speed=new BSlider(BRect(232,15,296,40),"speed","Speed [1x]",new BMessage('sped'),0,3,B_TRIANGLE_THUMB,B_FOLLOW_NONE);
	Speed->SetHashMarks(B_HASH_MARKS_BOTTOM);
	Speed->SetHashMarkCount(4);
	Speed->SetModificationMessage(new BMessage('sped'));
	
	CDRWBox->AddChild(ModePop);
	CDRWBox->AddChild(Blank);
	CDRWBox->AddChild(BlankStatus);
	CDRWBox->AddChild(BlankTxt);
	CDRWBox->AddChild(Speed);
	
	CDTypeBox->AddChild(CDRWBox);
	
	//SingleSession->SetValue(B_CONTROL_ON);
	
	Done=new BButton(BRect(5,30,97,55),"done","New",new BMessage('done'));
	Open=new BButton(BRect(102,30,195,55),"open","Open",new BMessage('open'));
	//Open->SetEnabled(false);
	OptionBox->AddChild(Done);
	OptionBox->AddChild(Open);

	char bufzer[1024];
	sprintf (bufzer,"%s/Projects/",MELT_PATH);

	BEntry myentry(bufzer,true);
	entry_ref* ref=new entry_ref();
	myentry.GetRef(ref);

	OpenPanel=new BFilePanel(B_OPEN_PANEL,&be_app_messenger,ref,B_DIRECTORY_NODE,false,NULL,NULL,true,true);

	AroundBox->AddChild(LogBox);
	AroundBox->AddChild(CDTypeBox);
	AroundBox->AddChild(OptionBox);
	
	AddChild(AroundBox);
};

/*
	**************************************************************
	WARNING: Note that the following function has never been used.
	And it's a shitty idea anyway.
	**************************************************************
*/

void CreateBufferFile()
{
	BAlert* myAlert=new BAlert("Melt Info",
	"Before you can use the BFS mode for the first time, a 650MB buffer file has to be made on one of your harddrives.","Cancel","Relocate","Create");
	
	int res=myAlert->Go();
	
	if (res==2)
	{
		int awinw=250;
		int awinh=50;
		
		BWindow* AddWindow=new BWindow(BRect(scrw/2-awinw/2,scrh/2-awinh/2,scrw/2+awinw/2,scrh/2+awinh/2),
							 "Melt: Working",B_MODAL_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE);
		
		BView* coverview=new BView(BRect(0,0,awinw,awinh),"Cover",B_FOLLOW_NONE,B_WILL_DRAW);
		BStatusBar* status=new BStatusBar(BRect(8,5,awinw-10,awinh-10),"status","Creating buffer...","0/650 MB");
		rgb_color grey={200,200,230};
		coverview->SetViewColor(grey);

		status->SetMaxValue(27);
	
		coverview->AddChild(status);
		AddWindow->AddChild(coverview);
		AddWindow->Show();
		
		char ddcommand[2048];
		sprintf(ddcommand,"dd if=/dev/zero of=/lot/ddtest.iso bs=1024k count=24");
		system(ddcommand);

		for (int i=0; i<27; i++)
		{
			AddWindow->Lock();
			status->Update(1);
			AddWindow->Unlock();
			system(ddcommand);
			sprintf (ddcommand,"dd if=/dev/zero of=/lot/ddtest.iso bs=1024k seek=%d count=1",25*i+24);
		};

		sprintf (ddcommand,"mkbfs 2048 /lot/ddtest.iso");
		system(ddcommand);
		system("mkdir /dev/melt-buffer/");
		system("mount /lot/ddtest.iso /dev/melt-buffer");

		AddWindow->Lock();
		AddWindow->Close();
		
		sprintf (MELT_RELOCATE,"/lot/ddtest.iso");
		
		sprintf (ddcommand,"%s/Temp/buf-location",MELT_PATH);
		FILE* vcd_relocate=fopen (ddcommand,"w");
		uint8 len=strlen(MELT_RELOCATE);
		res=fwrite(&len,1,1,vcd_relocate);
		if (res)
		{
			fwrite(MELT_RELOCATE,1,len,vcd_relocate);
		};
		MELT_RELOCATE[len]=0;
		fclose(vcd_relocate);
	};
};

int32 blink_control(void* p)
{
	blinkinfo* universe=(blinkinfo*)p;
	BStatusBar* bar=universe->bar;
	
	rgb_color green={100,255,100};
	rgb_color blue={100,100,255};
	int ub=0;

	while(universe->progress)
	{
		snooze(500000);
		ub=1-ub;
		if (ub)
		{
			bar->Window()->Lock();
			bar->SetBarColor(green);
			bar->Update(0);
			bar->Window()->Unlock();
		}
		else
		{
			bar->Window()->Lock();
			bar->SetBarColor(blue);
			bar->Update(0);
			bar->Window()->Unlock();
		};
	};
	return 0;
};

int32 blank_control(void* p)
{
	rgb_color red={200,0,0};
	rgb_color black={0,0,0};
	//rgb_color green={0,200,50};
	rgb_color blue={0,0,200};

	FILE* f;

	char* command=(char*)p;
	f=popen(command,"r");
	MeltNewProj* universe=((MeltApp*)be_app)->NewProjWin;

	universe->blink.progress=false;
	
	char buf[1024];
	char bufzer[1024];
	
	bool progress_mode=false;	
	bool burn_error=false;
	
	while (!feof(f) && !ferror(f))
	{
		buf[0]=0;

		fgets(buf,1024,f);

		Log(buf);

		if (!strncmp(buf,"Sense Code:",11))
		{
			strcpy (bufzer,buf+11);
			sprintf (buf,"The following error occured:\n\n%s",bufzer);
			burn_error=true;
			BAlert* myAlert=new BAlert("Melt Error",buf,"Damn");
			myAlert->Go();
			break;
		};
		if (!strncmp(buf,"Disk type:",10)) 
		{
			universe->Lock();
			universe->BlankTxt->Delete(0,500);
			universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
			universe->BlankTxt->Insert(buf);
			universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
			universe->Unlock();
		};
		if (!strncmp(buf,"Blanking",8) && !universe->blink.progress)
		{
			universe->Lock();
			universe->BlankTxt->Delete(0,500);
			universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
			universe->BlankTxt->Insert(buf);
			universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
			progress_mode=true;
			universe->BlankStatus->Reset();
			universe->BlankStatus->Update(100);
			universe->Unlock();
			universe->blink.progress=true;
			universe->blink.bar=universe->BlankStatus;
			
			resume_thread(spawn_thread(blink_control,"blink",5,&universe->blink));
		};
	};

	universe->blink.progress=false;

	if (!burn_error)
	{
		universe->Lock();
		universe->BlankTxt->Delete(0,500);
		universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
		universe->BlankTxt->Insert("Done.");
		universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
		universe->Unlock();
	}
	else
	{
		universe->Lock();
		universe->BlankTxt->Delete(0,500);
		universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
		universe->BlankTxt->Insert("Failure.");
		universe->BlankTxt->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
		universe->BlankStatus->SetBarColor(red);
		universe->BlankStatus->Update(0);
		universe->Unlock();
	};
	pclose(f);
	
	return 0;
};

void MeltNewProj::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case 'sped':
		{
			int valu=(int)Speed->Value();
			MELT_SPEED=(int)pow(2,valu);
			char buf[200];
			sprintf (buf,"Speed [%dx]",MELT_SPEED);
			Speed->SetLabel(buf);
		}
		break;
		case 'blnk':
		{
			char command[1024];

			sprintf (command,"cdrecord dev=%s speed=%d -v -blank=%s",MELT_DEVICE,MELT_SPEED,BLANK_CMD[MELT_BLANKMODE]);

			Log(command);

			thread_id blank=spawn_thread(blank_control,"blank",10,command);
			resume_thread(blank);
		}
		break;
		case 'open':
		{
			if (strncmp(MELT_DEVICE,"xxxxx",5))
			{
				OpenPanel->Show();
			}
			else
			{
				BAlert* myalert=new BAlert("Melt Warning","Please select a burner first.","OK");
				myalert->Go();
			};
		}
		break;
		case 'done':
			if (strncmp(MELT_DEVICE,"xxxxx",5))
			{
				/*char pbuf[2048];
				sprintf (pbuf,"%s/Temp/buf-location",MELT_PATH);
				FILE* vcd_relocate=fopen (pbuf,"r");
				uint8 len;
				int res=fread(&len,1,1,vcd_relocate);
				if (res)
				{
					fread(MELT_RELOCATE,1,len,vcd_relocate);
				};
				MELT_RELOCATE[len]=0;
				fclose(vcd_relocate);
				
				FILE* image=fopen(MELT_RELOCATE,"r");
				uint32 image_size=fseek(image,SEEK_END,0)+1;
				fclose (image);
				
				if (image_size!=665600)
				{
					CreateBufferFile();
				};*/
				
				MELT_MULTISESSION=(MultiSession->Value()==B_CONTROL_ON);
				if (UseAudio->Value()==B_CONTROL_ON) MELT_CDTYPE=1;
				be_app->PostMessage(new BMessage('opvw'));
			}
			else
			{
				BAlert* myalert=new BAlert("Melt Warning","Please select a burner first.","OK");
				myalert->Go();
			};
		break;
	};
	if ((msg->what&0xffffff00)=='dev\0')
	{
		uint8 savedrec=msg->what&0xff;
		strncpy(MELT_DEVICE,&Possible_Devices[savedrec*7],5);
		FILE* config=fopen("/boot/home/config/settings/Melt.cfg","w");
		fwrite(&savedrec,1,1,config);
		fclose(config);
		//SetTitle(MELT_DEVICE);
	};
	if ((msg->what&0xffffff00)=='blk\0')
	{
		uint8 bm=msg->what&0xff;
		MELT_BLANKMODE=bm;
		/*char buf[200];
		sprintf (buf,"Blankmode: %d (%s)",MELT_BLANKMODE,BLANK_CMD[MELT_BLANKMODE]);
		SetTitle(buf);*/
	};
};

bool MeltNewProj::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
};

// MeltTools --------------------------------------------------------------------------------------------------

int vwinw=400;
int vwinh=500;

MeltTools::MeltTools() : BView(BRect(0,0,vwinw,49),"tools",B_FOLLOW_NONE,B_WILL_DRAW)
{
	PrjName=new BTextControl(BRect(7,15,200,37),"name","Name:","Untitled",new BMessage('name'));
	PrjName->SetDivider(be_plain_font->StringWidth("Name: "));
	Save=new BButton(BRect(210,13,280,37),"save","Save As",new BMessage('save'));
	//Save->SetEnabled(false);
	Burn=new BButton(BRect(285,13,355,37),"burn","Done",new BMessage('burn'));

	char bufzer[1024];
	sprintf (bufzer,"%s/Projects/",MELT_PATH);

	BEntry myentry(bufzer,true);
	entry_ref* ref=new entry_ref();
	myentry.GetRef(ref);
	
	SavePanel=new BFilePanel(B_SAVE_PANEL,&be_app_messenger,ref,B_FILE_NODE|B_DIRECTORY_NODE,false,NULL,NULL,true,true);
	SavePanel->SetSaveText("Untitled");
	
	AddChild(PrjName);
	AddChild(Save);
	AddChild(Burn);
};

void MeltTools::Draw(BRect dummy)
{
	rgb_color light={250,250,255};
	rgb_color white={255,255,255};
	rgb_color black={0,0,0};
	rgb_color bgcol={200,200,200};
	
	BRect bnds=Bounds();
	
	float right=bnds.right;

	for (int y=0; y<49; y++)
	{
		light.red-=2;
		light.green-=2;
		light.blue-=1;
		SetHighColor(light);
		FillRect(BRect(0,y,right,y));
	};
	SetHighColor(black);
	FillRect(BRect(0,49,right,49));
	
	SetHighColor(bgcol);
	SetLowColor(bgcol);
	
	FillRect(BRect(5,8,right-5,40));
	SetHighColor(light);
	FillRect(BRect(4,7,right-5,7));
	FillRect(BRect(4,7,4,40));
	SetHighColor(black);
	FillRect(BRect(5,8,right-5,8));
	FillRect(BRect(5,8,5,40));
	
	SetHighColor(light);
	FillRect(BRect(right-5,8,right-5,40));
	FillRect(BRect(5,40,right-5,40));
	SetHighColor(white);
	FillRect(BRect(right-4,8,right-4,41));
	FillRect(BRect(5,41,right-5,41));
};

// MeltTrackList ----------------------------------------------------------------------------------------------

bool MeltAdjust(BListItem* that)
{
	that->SetHeight(34);
	return false;
};

MeltList::MeltList() : BListView(BRect(0,52,vwinw-B_V_SCROLL_BAR_WIDTH,vwinh),"Tracks",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL)
{
	// build menu
	TrackPop = new BPopUpMenu("context menu");
	TrackPop->SetRadioMode(false);
	TrackPop->AddItem(new BMenuItem("Move Upward", new BMessage('mvup')));
	TrackPop->AddItem(new BMenuItem("Remove", new BMessage('remt')));
	TrackPop->AddItem(new BMenuItem("Move Downward", new BMessage('mvdn')));
	
	if (MELT_CDTYPE==1) TrackPop->AddItem(new BMenuItem("Listen to this Track", new BMessage('hear')));
};

void MeltList::RePattern()
{
	int num=CountItems();
	if (num)
	{
		Global_Pattern=false;
		for (int i=1; i<=num; i++)
		{
			MeltItem* manip=(MeltItem*)ItemAt(i-1);
			manip->Pattern=Global_Pattern;
			Global_Pattern=!Global_Pattern;
		};
		Invalidate();
	};
};

void MeltList::MouseDown(BPoint where)
{
	uint32		buttons;
	BMessage*	msg=Window()->CurrentMessage();

	// retrieve the button state from the MouseDown message
	if (msg->FindInt32("buttons", (int32 *)&buttons) == B_NO_ERROR) {
		// find item at the mouse location
		int32 item = IndexOf(where);
		// make sure item is valid
		if ((item >= 0) && (item < CountItems())) {
			// if clicked with second mouse button, let's do a context-sensitive menu
			if (buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPoint	point = where;
				ConvertToScreen(&point);
				// select this item
				Select(item);
				TrackPop->Go(point, true, false, true);
				return;
			}
			if (buttons == B_PRIMARY_MOUSE_BUTTON)
			{
				int32 clicks = msg->FindInt32("clicks");
				if ((buttons == mLastButton) && (clicks > 1))
				{
					mClickCount++;
				}
				else mClickCount = 1;		// no, it's the first click of a new button
				mLastButton = buttons;		// remember what the last button pressed was
				
				if (clicks==2)
				{
					Window()->PostMessage(new BMessage('hear'));
				};
			};
		}
	}
	// either the user dbl-clicked an item or clicked in an area with no
	// items.  either way, let BListView take care of it
	BListView::MouseDown(where);
}

FILE* CLInput;
FILE* CLOutput;

int32 controller(void* p)
{
	char* command=(char*)p;
	CLInput=popen(command,"r");
	return 0;
};

void CreateImage(MeltList* list,entry_ref* ref)
{
	MeltVirtualCD* vc=((MeltApp*)be_app)->CDWin;

	BEntry entry(ref,true);
	BPath path;
	entry.GetPath(&path);
	char* temppath=(char*)path.Path();
	int len=strlen(temppath);
	char mypath[1024];
	strcpy(mypath,temppath);
	
	/*BNode node(&entry);
	
	fprintf (debfile,"Created node.\n");
	fclose(debfile);
	debfile=fopen("/boot/home/MELT.DBG","a");
	
	char typebuf[256];
	int nodelen=node.ReadAttr("BEOS:TYPE",0,0,typebuf,255);
	
	fprintf (debfile,"Read BEOS:TYPE attribute.\n");
	fclose(debfile);
	debfile=fopen("/boot/home/MELT.DBG","a");*/
	
	/*if (nodelen)
	{
		typebuf[nodelen]=0;
		
		bool isdir=false;
		
		if (!strcmp(typebuf,"application/x-vnd.Be-directory")) isdir=true;
		
		fprintf (debfile,"Dir Check 1.\n");
		fclose(debfile);
		debfile=fopen("/boot/home/MELT.DBG","a");
	}
	else
	{
		isdir=true;
	};*/
	
	bool isdir=false;
	bool isiso=false;
	
	char dumb[1];
	
	FILE* test=fopen(mypath,"r");
	int res=fread(&dumb,1,1,test);
	if (res!=1) isdir=true;
	fclose (test);
	
	if (mypath[len-1]=='/') isdir=true;
	
	uint32 i;
	int j=0;
	
	for (i=0; i<strlen(mypath); i++)
	{
		if (mypath[i]=='.') j=i;
	};
	i=j;
	
	if (!strcmp(&mypath[i],".iso") || !strcmp(&mypath[i],".img"))
	{
		isiso=true;
	};
	
	int override=0;
	
	char* trackname;
	char command[2048];
	
	for (i=strlen(mypath)-1; i>0; i--)
	{
		if (mypath[i]=='/') break;
	};
	
	trackname=&mypath[i+1];
	
	if (MELT_CDTYPE==0)
	{
		// ISO 9660 - Data track layout folder
	
		if (!isiso)
		{
			if (!isdir)
			{
				BAlert* myAlert=new BAlert("Melt Warning","Sorry, you can only drop folders or .iso and .img files onto the track list.\nSelect Override if this is a FAT32 (or similar) folder.","Override","OK");
				override=myAlert->Go();
				if (override) return;
			};

			sprintf (command,"/boot/common/bin/mkhybrid -a -r -J -V \"%s\" -o \"%s/Temp/%s.img\" \"%s\"",trackname,MELT_PATH,trackname,mypath);
			
			Log("Creating image file:");
			Log(command);

			int awinw=250;
			int awinh=50;
			
			BWindow* AddWindow=new BWindow(BRect(scrw/2-awinw/2,scrh/2-awinh/2,scrw/2+awinw/2,scrh/2+awinh/2),
								 "Melt: Working",B_MODAL_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE);
			
			BView* coverview=new BView(BRect(0,0,awinw,awinh),"Cover",B_FOLLOW_NONE,B_WILL_DRAW);
			BStatusBar* status=new BStatusBar(BRect(8,5,awinw-10,awinh-10),"status","Processing Folder...","Please Wait");
			rgb_color grey={200,200,230};
			coverview->SetViewColor(grey);

			coverview->AddChild(status);
			AddWindow->AddChild(coverview);
			AddWindow->Show();

			AddWindow->Lock();
			status->Update(100);
			AddWindow->Unlock();

			resume_thread(spawn_thread(controller,"Mkhybrid Controller",5,command));

			snooze(200000);
			char buf[1024];
			
			rgb_color green={100,255,100};
			rgb_color blue={100,100,255};

			blinkinfo info;
			info.bar=status;
			info.progress=true;

			resume_thread(spawn_thread(blink_control,"Mkhybrid Blink",5,&info));
			
			while (!feof(CLInput) && !ferror(CLInput))
			{
				snooze(200000);
				buf[0]=0;
				fgets(buf,1024,CLInput);
			};
			
			info.progress=false;

			pclose(CLInput);
			
			snooze(1000000);

			AddWindow->Lock();
			AddWindow->Close();

			sprintf (command,"%s/Temp/%s.img",MELT_PATH,trackname);
		}
		else
		{
			strcpy(command,mypath);
		};

		FILE* track=fopen(command,"r");
		fseek(track,0,SEEK_END);
		uint32 tracksize=(((ftell(track)+1)/1024)/1024);
		fclose(track);
		
		if (strlen(trackname)>40)
		{
			strcpy(trackname+40,"...");
		};
		
		if (!isiso) strcat(trackname,".img");
		
		char itembuf[200];
		sprintf (itembuf,"Data Track \"%s\" (%d MB)",trackname,(int)tracksize);

		MELT_CDSIZE+=tracksize;
		vc->MakeTitle();
		
		list->AddItem(new MeltItem(command,itembuf,vc->Icon[0],vc->Icon[1],vc->Icon[2]));
		list->DoForEach(MeltAdjust);
		list->Invalidate();
		is_saved=false;
	}
	else                   // Audio CD
	{
		bool accept=true;
	
		for (i=strlen(mypath)-1; i>0; i--)
		{
			if (mypath[i]=='/') break;
		};
		
		char* trackname=&mypath[i+1];
		int audiotype=0;
	
		int offset=0;
		bool got_it=false;
	
		if (!isdir)
		{
			FILE* examine=fopen (mypath,"r");
			fseek(examine,8,SEEK_SET);
			uint32 id=0;
			fread (&id,1,4,examine);
									
			if (id=='EVAW') 
			{
				audiotype=1;
				for (i=0; i<40; i++)
				{
					fread(&id,1,4,examine);
					if (id=='atad') got_it=true;
					if (got_it) break;
				};
				if (got_it)
				{
					offset=ftell(examine);
				}
				else
				{
					audiotype=0;
				};
			};
			/*if (id=='AIFF')
			{
				audiotype=2;
				for (i=0; i<40; i++)
				{
					fread(&id,1,4,examine);
					if (id=='SSND') got_it=true;
					if (got_it) break;
				};
				if (got_it)
				{
					offset=ftell(examine);
				}
				else
				{
					audiotype=0;
				};
			};*/
			if (audiotype==0) accept=false;
		}
		else
		{
			accept=false;
		};
		
		if (!accept)
		{
			BAlert* myAlert=new BAlert("Melt Warning",
									   "Please drop RIFF WAVE files (16 bit, 44kHz Stereo).\nYou could use SoundPlay (available on BeBits.com) to convert your files to this format if necessary.","OK");
			myAlert->Go();
			return;
		};

		FILE* track=fopen (mypath,"r");
		fseek (track,0,SEEK_END);
		uint32 audiobytes=ftell(track);
		fclose (track);
		
		// WAVE header=44 bytes
		// AIFF header=54 bytes
		
		if (audiotype==1)
		{
			audiobytes-=44;
		};
		
		int audiopad=2352*(1+audiobytes/2352)-audiobytes;

		printf ("audiobytes = %d\n",(int)audiobytes);
		printf ("audiobytes/2352+1 = %d\n",(int)1+audiobytes/2352);
		printf ("*2352 = %d\n",(int)2352*(1+audiobytes/2352));
		
		if (audiopad==2352) audiopad=0;
		
		if (audiopad)
		{
			char message[1000];
			sprintf (message,"The track size is not a multiple of 2352 bytes.\nDo you want Melt to resize the file to this boundary and fill the new part (%d bytes) with silence (zeroes)?",audiopad);

			BAlert* myAlert=new BAlert("Melt Warning",message,"No","Yes");
			int res=myAlert->Go();
			
			if (!res) {return;};
			
			uint8* nullbuffer=(uint8*)malloc(audiopad);
			memset(nullbuffer,0,audiopad);
			
			track=fopen(mypath,"a");
			fwrite(nullbuffer,1,audiopad,track);
			fclose(track);
			audiobytes+=audiopad;
		
			track=fopen(mypath,"r+");
			if (audiotype==1)
			{
				// RIFF WAVE
			
				fseek(track,0x28,SEEK_SET);
				fwrite(&audiobytes,1,4,track);
			}
			else
			{
				// AIFF
			
				fseek(track,0x2a,SEEK_SET);
				fwrite(((uint8*)(&audiobytes))+3,1,1,track);
				fwrite(((uint8*)(&audiobytes))+2,1,1,track);
				fwrite(((uint8*)(&audiobytes))+1,1,1,track);
				fwrite(((uint8*)(&audiobytes))+0,1,1,track);
			};
			fclose(track);
		};
		
		int tracksize=audiobytes/(1024*1024);

		MELT_CDSIZE+=tracksize;
		vc->MakeTitle();
		
		int sec=audiobytes/(4*44100); // 16 bit Stereo
		int min=sec/60;
		sec=(sec-min*60);
		
		char itrackname[200];
		strcpy(itrackname,trackname);
		
		if (strlen(itrackname)>40)
		{
			strcpy(itrackname+40,"...");
		};
		
		char itembuf[200];
		sprintf (itembuf,"Audio Track \"%s\" (%d MB %d:%d min)",itrackname,tracksize,min,sec);

		list->AddItem(new MeltItem(mypath,itembuf,vc->Icon[0],vc->Icon[1],vc->Icon[2]));
		list->DoForEach(MeltAdjust);
		list->Invalidate();
		is_saved=false;
	};
};

void MeltList::MessageReceived(BMessage* msg)
{
	entry_ref ref;
	int counter=0;
 	switch ( msg->what )
 	{
   		case B_SIMPLE_DATA:
   			while( msg->FindRef("refs", counter++, &ref) == B_OK )
   			{
   				CreateImage(this,&ref);
   			};
   		break;
	};
};

MeltItem::MeltItem(char* applyfile,char* applyname,const BBitmap* applyicon,const BBitmap* applyicon1,const BBitmap* applyicon2)
{
	strcpy(name,applyname);
	strcpy(fname,applyfile);
	icon=applyicon;
	icon_p=applyicon1;
	icon_s=applyicon2;
	Pattern=Global_Pattern;
	Global_Pattern=!Global_Pattern;
};

void MeltItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color rgbColor={255,255,255};
	rgb_color rgbSelectedColor={200,200,255};
	rgb_color rgbPatternColor={240,255,240};
	rgb_color black={0,0,0};
	
	const BBitmap* useicon=icon;
	
	if (IsSelected())
 	{  
        rgbColor = rgbSelectedColor;
        useicon=icon_s;
	}
	else
	{
		if (Pattern)
		{
			rgbColor = rgbPatternColor;
			useicon=icon_p;
		};
	};
	
	owner->SetHighColor(rgbColor); 
	owner->SetLowColor(rgbColor); 
	owner->FillRect(frame);
	
	owner->SetHighColor(black);
	
	owner->DrawBitmap(useicon,BPoint(1,frame.top+1));
	
	owner->MovePenTo(BPoint(34,frame.bottom-2));
	owner->DrawString(name);
};

// MeltBurn ----------------------------------------------------------------------------------------------

int bwinw=400;
int bwinh=310;

MeltBurn::MeltBurn() : BWindow(BRect(scrw/2-bwinw/2,scrh/2-bwinh/2,scrw/2+bwinw/2,scrh/2+bwinh/2),
									 "Melt: Burning CD-R",B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	MELT_SPEED=1;

	AroundBox=new BBox(BRect(0,0,bwinw,bwinh),"BetterStyle",B_FOLLOW_NONE,B_WILL_DRAW|B_FRAME_EVENTS,B_PLAIN_BORDER);

	LogBox=new BBox(BRect(3,3,bwinw-3,bwinh-130),"LogBox",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	LogBox->SetLabel("CDRecord Log:");
	
	RawCheck=new BCheckBox(BRect(3,bwinh-155,103,bwinh-140),"raw","Debug Mode",new BMessage('sraw'));
	
	LogView=new BTextView(BRect(8,18,winw-(14+B_V_SCROLL_BAR_WIDTH),bwinh-160),"cdrecord",BRect(5,15,winw-(24+B_V_SCROLL_BAR_WIDTH),winh-160),B_FOLLOW_NONE);
	LogView->SetStylable(true);
	LogView->MakeEditable(false);
	
	LogScroll=new BScrollView("logscroll",LogView,B_FOLLOW_NONE,0,false,true,B_FANCY_BORDER);
	
	OptionBox=new BBox(BRect(3,bwinh-125,bwinw-3,bwinh-3),"Options",B_FOLLOW_NONE,B_WILL_DRAW,B_FANCY_BORDER);
	OptionBox->SetLabel("Options:");
	
	Progress=new BStatusBar(BRect(5,15,bwinw-12,35),"progress","Idle...",NULL);
	
	Simulate=new BCheckBox(BRect(5,45,150,60),"sim","Simulate Only",new BMessage('csim'));
	Fixate=new BCheckBox(BRect(5,62,150,77),"fix","Fixate",new BMessage('cfix'));
	Eject=new BCheckBox(BRect(5,79,150,94),"eject","Eject CD",new BMessage('ceje'));	
	
	Speed=new BSlider(BRect(150,46,bwinw-13,80),"speed","Burning speed [1x]",new BMessage('sped'),0,3,B_TRIANGLE_THUMB,B_FOLLOW_NONE);
	Speed->SetHashMarks(B_HASH_MARKS_BOTTOM);
	Speed->SetHashMarkCount(4);
	Speed->SetModificationMessage(new BMessage('sped'));
	
	Start=new BButton(BRect(150,85,bwinw-13,110),"burn","Burn!",new BMessage('burn'));
	
	Fixate->SetValue(B_CONTROL_ON);
	
	OptionBox->AddChild(Progress);
	OptionBox->AddChild(Simulate);
	OptionBox->AddChild(Fixate);
	OptionBox->AddChild(Start);
	OptionBox->AddChild(Speed);
	OptionBox->AddChild(Eject);
	
	LogBox->AddChild(LogScroll);
	LogBox->AddChild(RawCheck);
	AroundBox->AddChild(LogBox);
	AroundBox->AddChild(OptionBox);
	AddChild(AroundBox);
};

void MeltBurn::MessageReceived(BMessage* msg)
{
	if (msg->what=='burn')
	{
		Burn(Creator);
	};
	if (msg->what=='sped')
	{
		int valu=(int)Speed->Value();
		MELT_SPEED=(int)pow(2,valu);
		char buf[200];
		sprintf (buf,"Burning speed [%dx]",MELT_SPEED);
		Speed->SetLabel(buf);
	};
};

void MeltBurn::Burn(BWindow* bwin)
{
	MeltVirtualCD* vcd=(MeltVirtualCD*)bwin;
	
	rgb_color red={200,0,0};
	rgb_color black={0,0,0};
	rgb_color green={0,200,50};
	rgb_color blue={0,0,200};
	
	char* MELT_MULTI="";
	char* MELT_EJECT="";
	
	if (Fixate->Value()==B_CONTROL_OFF) MELT_FIX="-nofix";
	if (Simulate->Value()==B_CONTROL_ON) MELT_DUMMY="-dummy";
	if (Eject->Value()==B_CONTROL_ON) MELT_EJECT="-eject";
	if (MELT_MULTISESSION) MELT_MULTI="-multi";
	if (MELT_CDTYPE) MELT_MULTI=" ";
	
	char command[2000];
	
	sprintf (command,"cdrecord dev=%s speed=%d %s %s %s %s -v",MELT_DEVICE,MELT_SPEED,MELT_FIX,MELT_DUMMY,MELT_EJECT,MELT_MULTI);
	int tracks=vcd->TrackList->CountItems();
	
	for (int i=0; i<tracks; i++)
	{
		if (MELT_CDTYPE) strcat(command," -audio");
		strcat (command," \"");
		strcat (command,((MeltItem*)vcd->TrackList->ItemAt(i))->fname);
		strcat (command,"\"");
	};
	
	Log("Burning CD:");
	Log(command);
	
	FILE* f=popen(command,"r");

	char buf[1024];
	char bufzer[1024];

	bool progress_mode=false;	
	bool burn_error=false;

	bool do_dump=(RawCheck->Value()==B_CONTROL_ON);
	
	while (!feof(f) && !ferror(f))
	{
		buf[0]=0;
		if (!progress_mode)
		{
			fgets(buf,1024,f);
		}
		else
		{
			fread(buf,1,33,f);
			buf[33]=0;
			if (buf[0]!=0x0d) progress_mode=false;
		};
		
		Log(buf);

		if (do_dump)
		{
			LogView->Insert(">");
			LogView->Insert(buf);
		};
		
		Lock();
		if (!strncmp(buf,"Sense Code:",11))
		{
			strcpy (bufzer,buf+11);
			sprintf (buf,"The following error occured:\n\n%s",bufzer);
			burn_error=true;
			BAlert* myAlert=new BAlert("Melt Error",buf,"Damn");
			myAlert->Go();
		};
		if (!strncmp(buf,"Disk type:",10)) 
		{
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
			LogView->Insert(buf);
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
		};
		if (!strncmp(buf,"Last chance",11))
		{
			LogView->Insert("Preparing...\n");
		};
		if (!strncmp(buf,"Starting new",12))
		{
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
			LogView->Insert(buf);
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
			progress_mode=true;
			Progress->Reset();
		};
		if (!strncmp(buf+1,"Track ",6))
		{
			strncpy (bufzer,buf+11,3);
			long done=strtol(bufzer,0,0);
			strncpy (bufzer,buf+18,3);
			long todo=strtol(bufzer,0,0);
			
			if (done)
			{
				Lock();
				Progress->SetBarColor(green);
				Progress->SetMaxValue(todo);
				Progress->Update(1,buf+1);
				Unlock();
			};
		};
		if (!strncmp(buf,"Fixating",8))
		{
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&blue);
			LogView->Insert(buf);
			LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
		};
		Unlock();
		
		LogView->ScrollToOffset(LogView->CountLines()*100);
	};
	Lock();
	if (!burn_error)
	{
		LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&green);
		LogView->Insert("Done.\n");
		LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
	}
	else
	{
		LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&red);
		LogView->Insert("Failure.\n");
		LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
		Lock();
		Progress->SetBarColor(red);
		Progress->Update(0,"Failure.");
		Unlock();
	};
	Unlock();
	pclose(f);
};

// MeltVirtualCD ----------------------------------------------------------------------------------------------

void Recolor(uint32* mem,int size,uint32 target)
{
	for (int i=0; i<size; i++) if ((*(mem+i)&0xffffff)==0xd8d8d8) *(mem+i)=target;
};

MeltVirtualCD::~MeltVirtualCD()
{
};

MeltVirtualCD::MeltVirtualCD() : BWindow (BRect(scrw/2-vwinw/2,scrh/2-vwinh/2,scrw/2+vwinw/2,scrh/2+vwinh/2),
										"Melt",B_DOCUMENT_WINDOW,0)
{
	rgb_color bgcol={200,200,200,0};
	
	ToolBox=new MeltTools();
	ToolBox->SetViewColor(bgcol);

	TrackList=new MeltList();
	TrackScroll=new BScrollView("trackscroll",TrackList,B_FOLLOW_ALL,0,false,true);

	TrackList->TrackPop->SetTargetForItems(this);

	if (MELT_CDTYPE==0)
	{
		Icon[0]=FetchStyleResource("label_2");
	}
	else
	{
		Icon[0]=FetchStyleResource("label_3");
	};

	Icon[1]=new BBitmap(Icon[0]->Bounds(),B_RGB32);
	Icon[2]=new BBitmap(Icon[0]->Bounds(),B_RGB32);
	int w=(int)Icon[0]->Bounds().right+1;
	int h=(int)Icon[0]->Bounds().bottom+1;
	memcpy(Icon[1]->Bits(),Icon[0]->Bits(),w*h*4);
	memcpy(Icon[2]->Bits(),Icon[0]->Bits(),w*h*4);
	
	Recolor ((uint32*)Icon[0]->Bits(),w*h,0xffffff);
	Recolor ((uint32*)Icon[1]->Bits(),w*h,0xf0fff0);
	Recolor ((uint32*)Icon[2]->Bits(),w*h,0xc8c8ff);
	
	AddChild(ToolBox);
	AddChild(TrackScroll);

	SetSizeLimits(vwinw,vwinw,100,768);
};

void MeltVirtualCD::MakeTitle()
{
	char* sstring;
	if (!MELT_CDTYPE)
	{
		if (MELT_MULTISESSION)
		{
			sstring="[ISO9660 multisession";
		}
		else
		{
			sstring="[ISO9660 single session";
		};
	}
	else
	{
		sstring="[AUDIO";
	};
	char buf[B_FILE_NAME_LENGTH+30];
	sprintf (buf,"Melt: %s %s, %d MB]",Title,sstring,MELT_CDSIZE);
	SetTitle(buf);
	ToolBox->SavePanel->SetSaveText(Title);
};

void MeltVirtualCD::MessageReceived(BMessage* msg)
{
	if (msg->what=='save')
	{
		ToolBox->SavePanel->Show();
	};
	if (msg->what=='name')
	{
		strcpy (Title,ToolBox->PrjName->Text());
		MakeTitle();
	};
	if (msg->what=='remt')
	{
		char killbuf[B_FILE_NAME_LENGTH+4];
		
		int32 selct=TrackList->CurrentSelection(0);
		MeltItem* item=(MeltItem*)(TrackList->ItemAt(selct));
		
		FILE* track=fopen(item->fname,"r");
		fseek(track,0,SEEK_END);
		int tracksize=ftell(track)/(1024*1024);
		fclose(track);
		
		MELT_CDSIZE-=tracksize;
		
		MakeTitle();
		
		BAlert* myAlert=new BAlert("Melt Request","Do you also want to erase this track from the disk?","Sure","Hell, No!");
		int res=myAlert->Go();
		
		if (res==0)
		{
			sprintf (killbuf,"rm \"%s\"",item->fname);
 			system(killbuf);
 		};
 		
 		TrackList->RemoveItem(item);
 		TrackList->RePattern();
 		is_saved=false;
 	};
 	if (msg->what=='mvup')
 	{
 		int32 selct=TrackList->CurrentSelection(0);
 		if (selct>0) TrackList->SwapItems(selct,selct-1);
 		TrackList->RePattern();
 		is_saved=false;
 	};
 	if (msg->what=='mvdn')
 	{
 		int32 selct=TrackList->CurrentSelection(0);
 		int32 max=TrackList->CountItems()-1;
 		if (selct<max) TrackList->SwapItems(selct,selct+1);
 		TrackList->RePattern();
 		is_saved=false;
 	};
	if (msg->what=='hear')
	{
		int32 selct=TrackList->CurrentSelection(0);
		MeltItem* myitem=(MeltItem*)TrackList->ItemAt(selct);
		
		BEntry playfile(myitem->fname);
		
		SetTitle(myitem->fname);
		
		if (playfile.InitCheck() == B_OK)
		{
			entry_ref playthis;
			playfile.GetRef(&playthis);
			BMessage* argmsg=new BMessage(B_REFS_RECEIVED);
			argmsg->AddRef("refs",&playthis);
			
			be_roster->Launch("audio/x-wav",argmsg);
		}
		else
		{
			SetTitle("BEntry Error!");
		};
		
		//be_roster->Launch(myitem->fname);
	};
	if (msg->what=='burn')
	{
		BurnWin=new MeltBurn();
		BurnWin->Creator=(BWindow*)this;
		BurnWin->Show();
	};
};

bool MeltVirtualCD::QuitRequested()
{
	int32 result=0;
	
	if (!is_saved)
	{
		BAlert* myAlert=new BAlert("Melt Warning","Do you really want do discard this Virtual CD?","Yes","No!");
		result=myAlert->Go();
	};
	
	if (!result)
	{
		SetTitle("Melt: Cleaning up...");

		delete Icon[0];
		delete Icon[1];
		delete Icon[2];
		
		char bufzer[1024];
		sprintf (bufzer,"rm \"%s/Temp/\"*.img",MELT_PATH);
		
		printf("%s\n",bufzer);
		
		system(bufzer);
		
		/*int tracks=TrackList->CountItems();
		
		char killcomm[B_FILE_NAME_LENGTH+5];
		
		for (int i=0; i<tracks; i++)
		{
			sprintf (killcomm,"rm \"%s\"",((MeltItem*)TrackList->ItemAt(i))->fname);
			system(killcomm);
		};*/

		BAlert* myAlert=new BAlert("Melt Request","Would you like to create another CD-R?","Yes","No");
		result=myAlert->Go();

		if (result)
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
		else
		{
			be_app->PostMessage(new BMessage('init'));
		};
		return true;
	}
	return false;
};

// MeltApp ----------------------------------------------------------------------------------------------

MeltApp::MeltApp() : BApplication(MELT_SIG)
{
	MELT_DEVICE=(char*)malloc(7);
	strcpy (MELT_DEVICE,"xxxxx");
	MELT_DEVICE[5]=0;
	MELT_DEVICE[6]=0;
};

void MeltApp::InitWindow()
{
	NewProjWin=new MeltNewProj();
	NewProjWin->BlankStatus->Update(100);
	NewProjWin->Show();
	
	rgb_color red={200,0,0};
	rgb_color black={0,0,0};
	
	NewProjWin->Lock();
	NewProjWin->LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&red);
	NewProjWin->LogView->Insert("Welcome to Melt™ v1.3 beta 3!\n");
	NewProjWin->LogView->Insert("©2000 Lukas Hartmann / Atomatrix\n\n");
	NewProjWin->LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
	NewProjWin->LogView->Insert("Melt runs:\n");
	NewProjWin->Unlock();

	FILE* f=popen("cdrecord -scanbus","r");
	
	char buf[1024];
	char bufzer[1024];
	char scsid[7];
	uint32 i;

	int RecorderCount=0;
	
	bool found_token=false;
	
	while (!feof(f) && !ferror(f))
	{
		buf[0]=0;
		fgets(buf,1024,f);
		NewProjWin->Lock();
		if (!strncmp(buf,"scsibus",7)) found_token=true;

		Log(buf);
		
		if (!strncmp(buf,"app",3)) break; // Skip memory leak debugger

		bool got_it=false;
		for (i=0; i<strlen(buf); i++)
		{
			// scan for first digit + comma
			if (buf[i]>='0' && buf[i]<='9')
			{
				if (buf[i+1]==',' || buf[i+2]==',')
				{
					char logbuf[1000];
					sprintf (logbuf,"*** Found digit and comma at %d.",(int)i);
					Log(logbuf);
					got_it=true; break;
				};
			};
		};
		if (got_it)
		{
			strncpy (scsid,&buf[i],6);
			for (i=0; i<strlen(buf); i++)
			{
				// scan for device info after ")"
				if (buf[i]==')') {break;};
			};
			strncpy(bufzer,&buf[i+2],strlen(buf)-(i+3));
			bufzer[strlen(buf)-(i+3)]=0;
			
			if (bufzer[0]!='*')
			{
				uint32 msg=('dev\0' | (RecorderCount));
				strncpy(&Possible_Devices[RecorderCount*7],scsid,6);
				
				if (scsid[3]==',')
				{
					scsid[5]=0;
				}
				else
				{
					scsid[6]=0;
				};
				
				char logbuf[1000];
				sprintf (logbuf,"*** Added scsid: \"%s\".",scsid);
				Log(logbuf);
				
				if (Possible_Devices[RecorderCount*7+3]==',')
				{
					Possible_Devices[RecorderCount*7+5]=0;
				}
				else
				{
					Possible_Devices[RecorderCount*7+6]=0;
				};
				
				for (i=0; i<strlen(bufzer); i++)
				{
					if (bufzer[i]=='\'') bufzer[i]=' ';
				};
				
				NewProjWin->Recorders->AddItem(new BMenuItem(bufzer,new BMessage(msg)));
			
				RecorderCount++;
			};
		};

		if (!found_token) NewProjWin->LogView->Insert(buf);
		NewProjWin->Unlock();
	};
	
	sprintf (buf,"\nMelt found %d devices on scsibus.",RecorderCount);
	if (RecorderCount==1) sprintf (buf,"\nMelt found 1 device on scsibus.");
	NewProjWin->Lock();
	NewProjWin->LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&red);
	NewProjWin->LogView->Insert(buf);
	NewProjWin->LogView->SetFontAndColor(0,0,be_plain_font,B_FONT_ALL,&black);
	
	if (RecorderCount)
	{
		FILE* config=fopen("/boot/home/config/settings/Melt.cfg","r");
		uint8 savedrec=0;
		if (fread(&savedrec,1,1,config)==1)
		{
			if (NewProjWin->Recorders->ItemAt(savedrec))
			{
				NewProjWin->Recorders->ItemAt(savedrec)->SetMarked(true);
				strncpy(MELT_DEVICE,&Possible_Devices[savedrec*7],6);
			};
		};
		fclose(config);
	};
	
	NewProjWin->Unlock();
	
	pclose(f);
};

void MeltApp::ReadyToRun()
{
	app_info info;
	be_app->GetAppInfo(&info);
	entry_ref ref=info.ref;

	BEntry entry(&ref,true);
	BPath path;
	entry.GetPath(&path);
	char* mypath=(char*)path.Path();
	int len=strlen(mypath);

	int j;

	for (j=len-1; j>0; j--)
	{
		if (mypath[j]=='/') break;
	};
	
	mypath[j]=0;
	
	MELT_PATH=(char*)malloc(strlen(mypath)+1);
	strcpy(MELT_PATH,mypath);

	strcat(mypath,"/Temp");
	BEntry testent(mypath);
	if (!testent.Exists())
	{
		BAlert* myalert=new BAlert("Melt Request","No Melt Temp folder found. Do you want Melt to create it?","No","Yes");
		int res=myalert->Go();
		if (res)
		{
			char buf[500];
			sprintf(buf,"mkdir %s",mypath);
			system(buf);
		};
	};

	InitWindow();
};

void MeltApp::RefsReceived(BMessage* msg)
{
	int pr_error=0;

	entry_ref ref;
	msg->FindRef("refs",&ref);
	BEntry entry(&ref,true);
	BPath path;
	entry.GetPath(&path);
	char* temppath=(char*)path.Path();

	char projpath[1024];
	strcpy (projpath,temppath);
	if (projpath[strlen(projpath)-1]!='/') strcat (projpath,"/");
	strcat (projpath,"melt-project");

	char trackpath[1024];
	char trackname[1024];

	FILE* project=fopen(projpath,"r");

	char projid[9];
	strcpy(projid,"xxxxxxxx");
	fread(projid,1,8,project);

	int j;

	if (!project)
	{
		pr_error=1;
	}
	else
	{
		MELT_CDTYPE=666;
		if (!strcmp(projid,"MELT9660")) MELT_CDTYPE=0;
		if (!strcmp(projid,"MELTAUDI")) MELT_CDTYPE=1;
		
		if (MELT_CDTYPE!=666)
		{
			NewProjWin->Lock();
			NewProjWin->Close();
			CDWin=new MeltVirtualCD();
			CDWin->Show();
		}
		else
		{
			 pr_error=2;
		};
	};

	while (!pr_error)
	{
		uint8 len;
		fread (&len,1,1,project);
		if (feof(project)) break;

		fread (trackpath,1,len,project);
		trackpath[len]=0;
		
		for (j=strlen(trackpath); j>0; j--)
		{
			if (trackpath[j]=='/') break;
		};

		strcpy(trackname,&trackpath[j+1]);

		FILE* track=fopen(trackpath,"r");
		if (track)
		{
			fseek(track,0,SEEK_END);
			uint32 tracksize=(((ftell(track))/1024)/1024);
			uint32 audiosize=ftell(track)-44;
			fclose(track);
			
			if (strlen(trackname)>40)
			{
				strcpy(trackname+40,"...");
			};
			
			char itembuf[200];
			if (MELT_CDTYPE==0)
			{
				sprintf (itembuf,"Data Track \"%s\" (%d MB)",trackname,(int)tracksize);
			}
			else
			{
				int sec=audiosize/(4*44100); // 16 bit Stereo
				int min=sec/60;
				sec-=min*60;
				sprintf (itembuf,"Audio Track \"%s\" (%d MB %d:%d min)",trackname,(int)tracksize,min,sec);
			};
		
			CDWin->Lock();
			CDWin->TrackList->AddItem(new MeltItem(trackpath,itembuf,CDWin->Icon[0],CDWin->Icon[1],CDWin->Icon[2]));
			CDWin->Unlock();
			
			MELT_CDSIZE+=tracksize;
		}
		else
		{
			char alertbuf[1024];
			sprintf (alertbuf,"The track \"%s\" couldn't be found.\nI assume thieves stole it.",trackpath);
			BAlert* myAlert=new BAlert("Banana",alertbuf,"Dial 911");
			myAlert->Go();
		};
	};
	fclose (project);
	if (!pr_error)
	{
		projpath[strlen(projpath)-13]=0;
		for (j=strlen(projpath); j>0; j--)
		{
			if (projpath[j]=='/') break;
		};
		strcpy(CDWin->Title,&projpath[j+1]);
		CDWin->Lock();
		CDWin->MakeTitle();
		CDWin->ToolBox->PrjName->SetText(CDWin->Title);
		CDWin->TrackList->DoForEach(MeltAdjust);
		CDWin->TrackList->Invalidate();
		CDWin->Unlock();
		is_saved=true;
	}
	else
	{
		BAlert* myAlert=new BAlert("Melt Error","The selected folder doesn't include a valid Melt project.\nComputers are terrible sometimes.\nSame for users.","Panic 2000");
		myAlert->Go();
	};
};

void MeltApp::MessageReceived(BMessage* msg)
{
	if (msg->what == 'init')
	{
		snooze(1000000);
		InitWindow();
	};
	if (msg->what == B_SAVE_REQUESTED)
	{
		CDWin->SetTitle("Melt: Saving...");
		entry_ref ref;
		const char* name;
		msg->FindRef("directory", &ref);
		msg->FindString("name", &name);
		BEntry entry;
		entry.SetTo(&ref);
		BPath path;
		entry.GetPath(&path);
		path.Append(name);
		
		char bufzer[1024];
		char buf2er[1024];
		sprintf (bufzer,"mkdir -p \"%s\"",path.Path());
		system (bufzer);
		
		sprintf (bufzer,"%s/melt-project",path.Path());
		
		FILE* projfile=fopen(bufzer,"w");
		
		char projid[9];
		strcpy(projid,"MELT9660");
		if (MELT_CDTYPE==1) strcpy(projid+4,"AUDI");
		
		fwrite(projid,1,8,projfile);
		
		int tracks=CDWin->TrackList->CountItems();
		
		int j;
		
		bool do_move=false;
		
		if (MELT_CDTYPE==0)
		{
			BAlert* alert=new BAlert("Melt Request","Do you want Melt to copy or move the track images into the project folder?","Copy","Move");
			int res=alert->Go();
			if (res) do_move=true;
		}
		else
		{
			BAlert* alert=new BAlert("Melt Request","Would you like Melt to move the track images into the project folder or leave them at their origin?","Move","Leave");
			int res=alert->Go();
			if (!res) do_move=true;
		};
		
		for (int i=0; i<tracks; i++)
		{
			char* thename=((MeltItem*)CDWin->TrackList->ItemAt(i))->fname;
			if (MELT_CDTYPE==0 || do_move)
			{
				for (j=strlen(thename); j>0; j--)
				{
					if (thename[j]=='/') break;
				};
				sprintf (bufzer,"%s/%s",path.Path(),&thename[j+1]);
				if (!do_move)
				{
					sprintf (buf2er,"cp \"%s\" \"%s\"",thename,path.Path());
				}
				else
				{
					sprintf (buf2er,"mv \"%s\" \"%s\"",thename,path.Path());
					printf ("%s\n",buf2er);
				};
				
				/*BAlert* myAlert=new BAlert("Melt Alert",buf2er,"D'Oh");
				myAlert->Go();*/
				
				system(buf2er);
				
				strcpy(((MeltItem*)CDWin->TrackList->ItemAt(i))->fname,bufzer);
			}
			else
			{
				strcpy(bufzer,thename);
			};
			uint8 len=strlen(bufzer);
			fwrite (&len,1,1,projfile);
			fputs (bufzer,projfile);
		};
		
		fclose (projfile);
		CDWin->MakeTitle();
		is_saved=true;
	};
	switch (msg->what)
	{
		case 'opvw':
			NewProjWin->Lock();
			NewProjWin->Close();
			CDWin=new MeltVirtualCD();
			CDWin->Show();
			strcpy (CDWin->Title,"Untitled");
			CDWin->MakeTitle();
		break;
	};
};

int main(int argc,char** argv)
{
	DO_LOG=false;
	if (argc>1) if (!strcmp(argv[1],"-debug")) DO_LOG=true;

	be_app=new MeltApp();
	be_app->Run();
	
	return 0;
};
