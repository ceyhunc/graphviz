//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

#include "dthdr.h"

using namespace cdttest;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

typedef struct {
	Dtlink_t link;
	int freq;
	char *name;
} Token_t;

Dtdisc_t td={offsetof(Token_t,name),-1,0};

Token_t *newtoken(char *s)
{
	Token_t *tk;
	tk=(Token_t *)malloc(sizeof(Token_t));
	tk->name=(char *)malloc(strlen(s)+1);
	strcpy_s(tk->name, strlen(s)+1, s);
	tk->freq=1;
	return tk;
}

void cdttest::MainPage::testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Token_t *tk;
	Dt_t *dt=dtopen(&td,Dtset);
	char *a[]={"bir","iki","bir"};
	for (int i = 0; i < sizeof(a)/sizeof(a[0]); i++)
	{
		if ((tk=(Token_t *)dtmatch(dt,a[i])))
		{
			tk->freq+=1;
		}
		else
			dtinsert(dt,newtoken(a[i]));

	}
	
	for (tk=(Token_t *)dtfirst(dt); tk; tk=(Token_t *)dtnext(dt,tk))
	{
		char a[80];
		sprintf(a, "%s %d", tk->name, tk->freq);

		wchar_t s[80+1];
		mbstowcs(s,a,strlen(a));
		s[strlen(a)]=0;
		testResult->Text+=ref new String(s);
		testResult->Text+="\r\n";
	}

}
