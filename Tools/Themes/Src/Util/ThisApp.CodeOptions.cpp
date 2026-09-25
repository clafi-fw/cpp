module ThisApp.CodeOptions;

import ThisApp.Consts;
import ThisApp.ThemeToCppCode;

import ClaFi.Dom;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ThisApp::CodeOptions
{
    using namespace ClaFi;

    // THESE ARE CONSTRUCTED BEFORE MAIN, so nothing built here may touch the platform, the theme
    // or the text engine - none of them exists yet. A Text is a string and its markers, and an
    // EventComponent connects its handlers into a map of its own, so both are safe; anything
    // added to an action below has to answer the same question.

    namespace
    {
        CodeContent s_content{ CodeContent::Full };
        CodeScope s_scope{ CodeScope::ClassDeclarations };

        // A click writes through the form it arrived on: an action answering for itself is handed
        // no control, and the form is the one thing both events of a click carry.
        void setContent(CodeContent value, FormBase& form)
        {
            s_content = value;
            (form.appContext().config() / k_codeContentAttrName).set(value);
            // Two commands state one answer, so the one that lost the mark has to be asked again
            // as much as the one that took it. Asking an action refreshes every presenter of it,
            // on every page and in every tab.
            onlyDifferences.invalidateState();
            full.invalidateState();
        }

        void setScope(CodeScope value, FormBase& form)
        {
            s_scope = value;
            (form.appContext().config() / k_codeScopeAttrName).set(value);
            asClassDeclarations.invalidateState();
            asClassMethod.invalidateState();
            asOutsideClass.invalidateState();
        }
    }

    CodeContent content()
    {
        return s_content;
    }

    CodeScope scope()
    {
        return s_scope;
    }

    void restore(const Dom::Section& appConfig)
    {
        s_content = (appConfig / k_codeContentAttrName).get<CodeContent>();
        s_scope = (appConfig / k_codeScopeAttrName).get<CodeScope>();
    }

    Action onlyDifferences{
        Text{ L"Only differences" },
        Action::OnGetState{ [](GetActionStateEvent& event) {
            event.claim({ .selected = s_content == CodeContent::Differences });
        } },
        Action::OnClick{ [](ActionClickEvent& event) {
            setContent(CodeContent::Differences, event.form);
        } }
    };

    Action full{
        Text{ L"Full" },
        Action::OnGetState{ [](GetActionStateEvent& event) {
            event.claim({ .selected = s_content == CodeContent::Full });
        } },
        Action::OnClick{ [](ActionClickEvent& event) {
            setContent(CodeContent::Full, event.form);
        } }
    };

    Action asClassDeclarations{
        Text{ L"Class declarations" },
        Action::OnGetState{ [](GetActionStateEvent& event) {
            event.claim({ .selected = s_scope == CodeScope::ClassDeclarations });
        } },
        Action::OnClick{ [](ActionClickEvent& event) {
            setScope(CodeScope::ClassDeclarations, event.form);
        } }
    };

    Action asClassMethod{
        Text{ L"Class method" },
        Action::OnGetState{ [](GetActionStateEvent& event) {
            event.claim({ .selected = s_scope == CodeScope::ClassMethod });
        } },
        Action::OnClick{ [](ActionClickEvent& event) {
            setScope(CodeScope::ClassMethod, event.form);
        } }
    };

    Action asOutsideClass{
        Text{ L"Outside the class" },
        Action::OnGetState{ [](GetActionStateEvent& event) {
            event.claim({ .selected = s_scope == CodeScope::OutsideClass });
        } },
        Action::OnClick{ [](ActionClickEvent& event) {
            setScope(CodeScope::OutsideClass, event.form);
        } }
    };

}
