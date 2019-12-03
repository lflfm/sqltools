/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2017 Aleksey Kochetov

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

/*
2017-12-10 Popuip Viewer is based on the code from sqltools++ by Randolf Geist
*/

#include "stdafx.h"
#include "resource.h"
#include "PopupViewer.h"
#include "COMMON\VisualAttributes.h"
#include "COMMON/GUICommandDictionary.h"
#include "GridView.h"

using namespace Common;

namespace OG2 /* OG = OpenGrid V2 */
{

    void PopupFrameWnd::SetPopupText (const std::string& text)
{ 
    m_text = text;

    if (m_EditBox.m_hWnd) 
        m_EditBox.SetWindowText(text.c_str()); 

    if (m_hmlViewer.m_hWnd) 
    {
        //m_hmlViewer.Navigate2("E:\\_CPP\\test.xml");
        //m_hmlViewer.LoadHTMLfromString(text.c_str());
        m_hmlViewer.Navigate2(_T("about:blank"), NULL, NULL, NULL, NULL);
        
        IDispatch* pDocDisp = m_hmlViewer.GetHtmlDocument();

        if (IDispatch* pDocDisp = m_hmlViewer.GetHtmlDocument())
        {
            IHTMLDocument2 *pDoc = NULL;

            HRESULT hr = pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);

            if (SUCCEEDED(hr) && pDoc != NULL)
            {

                // construct text to be written to browser as SAFEARRAY
                SAFEARRAY *safe_array = SafeArrayCreateVector(VT_VARIANT,0,1);

                VARIANT *variant;
                // string contains the HTML data.
                // convert char* string to OLEstring

                CComBSTR bstrTmp = text.c_str();

                SafeArrayAccessData(safe_array,(LPVOID *)&variant);
                variant->vt = VT_BSTR;
                variant->bstrVal = bstrTmp;
                SafeArrayUnaccessData(safe_array);

                pDoc->clear();
                // write SAFEARRAY to browser document to append string
                pDoc->write(safe_array);
                pDoc->close();

                //Detach CComBSTR since string will be freed by SafeArrayDestroy
                bstrTmp.Detach();

                //free safe_array
                SafeArrayDestroy(safe_array);

                //release document
                pDoc->Release();
            }
            pDocDisp->Release();
        }
    }

}

//#import <mshtml.tlb>
//_COM_SMARTPTR_TYPEDEF(IPersistStreamInit  , __uuidof(IPersistStreamInit));
//void HtmlView::LoadHTMLfromString (const string& strHTML)
//{
//        HRESULT hr;
//        void* pHTML = NULL;
//        IDispatch* pHTMLDoc = NULL;
//        IStream* pHTMLStream = NULL;
//        IPersistStreamInitPtr pPersistStreamInit = NULL;
//
//        HGLOBAL hHTML = GlobalAlloc(GPTR, strHTML.size()+1);
//        pHTML = GlobalLock(hHTML);
//        memcpy(pHTML, strHTML.c_str(), strHTML.size()+1);
//        GlobalUnlock(hHTML);
//
//        hr = CreateStreamOnHGlobal(hHTML, TRUE, &pHTMLStream);
//        if (SUCCEEDED(hr))
//        {
//                // Get the underlying document.
//                m_pBrowserApp->get_Document(&pHTMLDoc);
//                if (!pHTMLDoc)
//                {
//                        // Ensure we have a document.
//                        Navigate2(_T("about:blank"), NULL, NULL, NULL, NULL);
//                        m_pBrowserApp->get_Document(&pHTMLDoc);
//                }
//
//                if (pHTMLDoc)
//                {
//                        hr = pHTMLDoc->QueryInterface( IID_IPersistStreamInit, (void**)&pPersistStreamInit );
//                        if (SUCCEEDED(hr))
//                        {
//                                hr = pPersistStreamInit->InitNew();
//                                if (SUCCEEDED(hr))
//                                {
//                                        // Load the HTML from the stream.
//                                        hr = pPersistStreamInit->Load(pHTMLStream);
//                                }
//
//                                pPersistStreamInit->Release();
//                        }
//
//                        pHTMLDoc->Release();
//                }
//
//                pHTMLStream->Release();
//        }
//
//        return;
//}

}//namespace OG2
