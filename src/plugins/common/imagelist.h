/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2021 Andrea Zanellato <redtid3@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <plugin.h>

#include <ticpp.h>

#include <wx/imaglist.h>

namespace ImageList {

void RestoreSize(wxObject* wxobject, IManager* manager)
{
    IObject* imageListObject = manager->GetIObject(wxobject);

    wxImageList* imageList = wxDynamicCast(wxobject, wxImageList);
    if (!imageList)
        return;

    wxSize imageListsize = imageListObject->GetPropertyAsSize("size");
    int imageListwidth = imageListsize.GetWidth();
    int imageListheight = imageListsize.GetHeight();
    size_t count = manager->GetChildCount(wxobject);

    for (size_t i = 0; i < count; i++) {
        wxObject* bmpItem = manager->GetChild(wxobject, i);
        IObject* bmpObject = manager->GetIObject(bmpItem);

        if (bmpObject->GetClassName() == "bitmapitem") {
            wxBitmap bmp = bmpObject->GetPropertyAsBitmap("bitmap");
            int bmpWidth = bmp.GetWidth();
            int bmpHeight = bmp.GetHeight();

            if (bmpWidth > 0 && bmpHeight > 0 && bmp.IsOk()) {

                if ((imageListwidth < 1) || (imageListheight < 1)) {
                    // FIXME! This returns always 22x22 because default.xpm
                    imageListwidth = bmpWidth;
                    imageListheight = bmpHeight;
                    manager->ModifyProperty(
                        imageList, "size",
                        wxString::Format("%i,%i", imageListwidth, imageListheight), false);
                }
                if ((bmpWidth != imageListwidth) || (bmpHeight != imageListheight)) {
                    wxImage image = bmp.ConvertToImage();
                    bmp = wxBitmap(image.Scale(imageListwidth, imageListheight));
                }
                imageList->Add(bmp);
            }
        }
    }
    if (!count)
        manager->ModifyProperty(imageList, "size", "16,16", false);
}
} // namespace ImageList

namespace BitmapItem {

ticpp::Element* ToXrc(IObject* obj, ticpp::Element* bmpItem)
{
    wxString bmpProp = obj->GetPropertyAsString("bitmap");

    if (bmpProp.empty())
        return bmpItem;

    wxString filename = bmpProp.AfterFirst(';');
    if (filename.empty())
        return bmpItem;

    if (bmpProp.size() < (filename.size() + 2))
        return bmpItem;

    // TODO: Load From (Icon) Resource
    if (bmpProp.StartsWith("Load From File")
        || bmpProp.StartsWith("Load From Embedded File")
        || bmpProp.StartsWith("Load From XRC")) {
        bmpItem->SetText(filename.Trim().Trim(false));
    } else if (bmpProp.StartsWith("Load From Art Provider")) {
        wxString stockId = filename.BeforeFirst(';').Trim().Trim(false).mb_str(wxConvUTF8);
        wxString stockClient = filename.AfterFirst(';').Trim().Trim(false).mb_str(wxConvUTF8);

        if (stockId.empty() || stockClient.empty())
            return bmpItem;

        bmpItem->SetAttribute("stock_id", stockId);
        bmpItem->SetAttribute("stock_client", stockClient);
        bmpItem->SetText("undefined.png"); // fallback image
    }
    return bmpItem;
}
} // namespace BitmapItem
