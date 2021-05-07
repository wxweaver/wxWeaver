#include <wx/html/forcelnk.h>
#include <wx/html/m_templ.h>

#include "../rad/appdata.h"


FORCE_LINK_ME(m_wxweaver)


TAG_HANDLER_BEGIN(wxWeaverVersion, "WXWEAVER-VERSION")

TAG_HANDLER_PROC(WXUNUSED(tag))
{
	auto* cell = new wxHtmlWordCell(VERSION, *m_WParser->GetDC());
	m_WParser->ApplyStateToCell(cell);
	m_WParser->GetContainer()->InsertCell(cell);

	return false;
}

TAG_HANDLER_END(wxWeaverVersion)


TAG_HANDLER_BEGIN(wxWeaverRevision, "WXWEAVER-REVISION")

TAG_HANDLER_PROC(WXUNUSED(tag))
{
	auto* cell = new wxHtmlWordCell(REVISION, *m_WParser->GetDC());
	m_WParser->ApplyStateToCell(cell);
	m_WParser->GetContainer()->InsertCell(cell);

	return false;
}

TAG_HANDLER_END(wxWeaverRevision)


TAGS_MODULE_BEGIN(wxWeaver)

TAGS_MODULE_ADD(wxWeaverVersion)
TAGS_MODULE_ADD(wxWeaverRevision)

TAGS_MODULE_END(wxWeaver)
