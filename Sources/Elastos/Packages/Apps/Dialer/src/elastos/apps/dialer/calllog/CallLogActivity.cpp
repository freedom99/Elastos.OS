
#include "R.h"
#include "elastos/apps/dialer/calllog/CallLogActivity.h"
#include "elastos/apps/dialer/calllog/CCallLogFragment.h"
#include "elastos/apps/dialer/calllog/ClearCallLogDialog.h"
#include "elastos/apps/dialer/calllog/CCallLogQueryHandler.h"
#include "elastos/apps/dialer/voicemail/CVoicemailStatusHelperImpl.h"
#include "Elastos.Droid.App.h"
#include "Elastos.Droid.Content.h"
#include "Elastos.Droid.Provider.h"
#include "Elastos.Droid.Widget.h"
#include <elastos/droid/R.h>

using Elastos::Droid::App::IActionBar;
using Elastos::Droid::App::IFragmentManager;
using Elastos::Droid::Content::IContentResolver;
using Elastos::Droid::Content::CIntent;
using Elastos::Droid::Os::CHandler;
using Elastos::Droid::Provider::ICalls;
using Elastos::Droid::Widget::IAdapter;
using Elastos::Apps::Dialer::Voicemail::IVoicemailStatusHelperImpl;
using Elastos::Apps::Dialer::Voicemail::CVoicemailStatusHelperImpl;

namespace Elastos {
namespace Apps {
namespace Dialer {
namespace CallLog {

const Int32 CallLogActivity::WAIT_FOR_VOICEMAIL_PROVIDER_TIMEOUT_MS = 300;
const Int32 CallLogActivity::TAB_INDEX_ALL = 0;
const Int32 CallLogActivity::TAB_INDEX_MISSED = 1;
const Int32 CallLogActivity::TAB_INDEX_VOICEMAIL = 2;

const Int32 CallLogActivity::TAB_INDEX_COUNT_DEFAULT = 2;
const Int32 CallLogActivity::TAB_INDEX_COUNT_WITH_VOICEMAIL = 3;

//=================================================================
// CallLogActivity::ViewPagerAdapter
//=================================================================
// CallLogActivity::ViewPagerAdapter::ViewPagerAdapter(
//     /* [in] */ IFragmentManager* fm,
//     /* [in] */ CallLogActivity* host)
//     : mHost(host)
// {
//     FragmentPagerAdapter(fm);
// }

// ECode CallLogActivity::ViewPagerAdapter::GetItem(
//     /* [in] */ Int32 position,
//     /* [out] */ IFragment** item)
// {
//     VALIDATE_NOT_NULL(item);
//     switch (position) {
//         case TAB_INDEX_ALL:
//             CCallLogFragment::New(ICallLogQueryHandler::CALL_TYPE_ALL,
//                     (ICallLogFragment**)&mAllCallsFragment);
//             *item = IFragment::Probe(mAllCallsFragment);
//             REFCOUNT_ADD(*item);
//             return NOERROR;
//         case TAB_INDEX_MISSED:
//             CCallLogFragment::New(ICalls::MISSED_TYPE,
//                     (ICallLogFragment**)&mMissedCallsFragment);
//             *item = IFragment::Probe(mMissedCallsFragment);
//             REFCOUNT_ADD(*item);
//             return NOERROR;
//         case TAB_INDEX_VOICEMAIL:
//             CCallLogFragment::New(ICalls::VOICEMAIL_TYPE,
//                     (ICallLogFragment**)&mVoicemailFragment);
//             *item = IFragment::Probe(mVoicemailFragment);
//             REFCOUNT_ADD(*item);
//             return NOERROR;
//     }

//     // throw new IllegalStateException("No fragment at position " + position);
//     Logger::E(String("CallLogActivity"), "No fragment at position %d", position);
//     return E_ILLEGAL_STATE_EXCEPTION;
// }

// ECode CallLogActivity::GetPageTitle(
//     /* [in] */ Int32 position,
//     /* [out] */ ICharSequence** title)
// {
//     VALIDATE_NOT_NULL(title);
//     *title = CoreUtils::Convert(mHost->mTabTitles[position]);
//     REFCOUNT_ADD(*title);
//     return NOERROR;
// }

// ECode CallLogActivity::GetCount(
//     /* [out] */ Int32* count)
// {
//     VALIDATE_NOT_NULL(count);
//     *count = mHost->mHasActiveVoicemailProvider ?
//             TAB_INDEX_COUNT_WITH_VOICEMAIL : TAB_INDEX_COUNT_DEFAULT;
//     return NOERROR;
// }

//=================================================================
// CallLogActivity::WaitForVoicemailTimeoutRunnable
//=================================================================
CallLogActivity::WaitForVoicemailTimeoutRunnable::WaitForVoicemailTimeoutRunnable(
    /* [in] */ CallLogActivity* host)
    : mHost(host)
{}

ECode CallLogActivity::WaitForVoicemailTimeoutRunnable::Run()
{
    assert(0 && "TODO");
    // mHost->mViewPagerTabs->SetViewPager(mHost->mViewPager);
    // mHost->mViewPager->SetCurrentItem(TAB_INDEX_ALL);
    mHost->mSwitchToVoicemailTab = FALSE;
    return NOERROR;
}

//=================================================================
// CallLogActivity
//=================================================================
// TODO:
// CAR_INTERFACE_IMPL_2(CallLogActivity, AnalyticsActivity, ICallLogActivity, ICallLogQueryHandlerListener);
CAR_INTERFACE_IMPL_2(CallLogActivity, Activity, ICallLogActivity, ICallLogQueryHandlerListener);

CallLogActivity::CallLogActivity()
{
    AutoPtr<WaitForVoicemailTimeoutRunnable> runnable = new WaitForVoicemailTimeoutRunnable(this);
    mWaitForVoicemailTimeoutRunnable = IRunnable::Probe(runnable);
}

ECode CallLogActivity::DispatchTouchEvent(
    /* [in] */ IMotionEvent* ev,
    /* [out] */ Boolean* isConsumed)
{
    Int32 action;
    ev->GetAction(&action);
    if (action == IMotionEvent::ACTION_DOWN) {
        Float rawX, rawY;
        ev->GetRawX(&rawX);
        ev->GetRawY(&rawY);
    assert(0 && "TODO");
        // TouchPointManager::GetInstance()->SetPoint((Int32)rawX, (Int32)rawY);
    }
    assert(0 && "TODO");
    // return AnalyticsActivity::DispatchTouchEvent(ev, isConsumed);
    return NOERROR;
}

ECode CallLogActivity::OnCreate(
    /* [in] */ IBundle* savedInstanceState)
{
    assert(0 && "TODO");
    // AnalyticsActivity::OnCreate(savedInstanceState);

    CHandler::New((IHandler**)&mHandler);

    SetContentView(R::layout::call_log_activity);
    GetWindow()->SetBackgroundDrawable(NULL);

    AutoPtr<IActionBar> actionBar;
    GetActionBar((IActionBar**)&actionBar);
    actionBar->SetDisplayShowHomeEnabled(TRUE);
    actionBar->SetDisplayHomeAsUpEnabled(TRUE);
    actionBar->SetDisplayShowTitleEnabled(TRUE);
    actionBar->SetElevation(0);

    Int32 startingTab = TAB_INDEX_ALL;
    AutoPtr<IIntent> intent;
    GetIntent((IIntent**)&intent);
    if (intent != NULL) {
        Int32 callType;
        intent->GetInt32Extra(ICalls::EXTRA_CALL_TYPE_FILTER, -1, &callType);
        if (callType == ICalls::MISSED_TYPE) {
            startingTab = TAB_INDEX_MISSED;
        }
        else if (callType == ICalls::VOICEMAIL_TYPE) {
            startingTab = TAB_INDEX_VOICEMAIL;
        }
    }

    mTabTitles = ArrayOf<String>::Alloc(TAB_INDEX_COUNT_WITH_VOICEMAIL);
    String alltitle;
    GetString(R::string::call_log_all_title, &alltitle);
    mTabTitles->Set(0, alltitle);
    String missedtitle;
    GetString(R::string::call_log_missed_title, &missedtitle);
    mTabTitles->Set(1, missedtitle);
    String voicemailtitle;
    GetString(R::string::call_log_voicemail_title, &voicemailtitle);
    mTabTitles->Set(2, voicemailtitle);

    AutoPtr<IView> view;
    FAIL_RETURN(FindViewById(R::id::call_log_pager, (IView**)&view));
    assert(0 && "TODO");
    // mViewPager = IViewPager::Probe(view);

    AutoPtr<IFragmentManager> manager;
    GetFragmentManager((IFragmentManager**)&manager);
    assert(0 && "TODO");
    // mViewPagerAdapter = new ViewPagerAdapter(manager);
    // mViewPager->SetAdapter(mViewPagerAdapter);
    // mViewPager->SetOffscreenPageLimit(2);

    view = NULL;
    FAIL_RETURN(FindViewById(R::id::viewpager_header, (IView**)&view));
    assert(0 && "TODO");
    // mViewPagerTabs = IViewPagerTabs::Probe(view);
    // mViewPager->SetOnPageChangeListener(mViewPagerTabs);

    if (startingTab == TAB_INDEX_VOICEMAIL) {
        // The addition of the voicemail tab is an asynchronous process, so wait till the tab
        // is added, before attempting to switch to it. If the querying of CP2 for voicemail
        // providers takes too long, give up and show the first tab instead.
        mSwitchToVoicemailTab = TRUE;
        Boolean result;
        mHandler->PostDelayed(mWaitForVoicemailTimeoutRunnable,
                WAIT_FOR_VOICEMAIL_PROVIDER_TIMEOUT_MS, &result);
    }
    else {
    assert(0 && "TODO");
        // mViewPagerTabs->SetViewPager(mViewPager);
        // mViewPager->SetCurrentItem(startingTab);
    }

    CVoicemailStatusHelperImpl::New((IVoicemailStatusHelperImpl**)&mVoicemailStatusHelper);
    return NOERROR;
}

ECode CallLogActivity::OnResume()
{
    assert(0 && "TODO");
    // AnalyticsActivity::OnResume();
    AutoPtr<IContentResolver> resolver;
    GetContentResolver((IContentResolver**)&resolver);
    AutoPtr<ICallLogQueryHandler> callLogQueryHandler;
    CCallLogQueryHandler::New(resolver,
            (ICallLogQueryHandlerListener*)this, (ICallLogQueryHandler**)&callLogQueryHandler);
    callLogQueryHandler->FetchVoicemailStatus();

    return NOERROR;
}

ECode CallLogActivity::OnCreateOptionsMenu(
    /* [in] */ IMenu* menu,
    /* [out] */ Boolean* allowToShow)
{
    AutoPtr<IMenuInflater> inflater;
    GetMenuInflater((IMenuInflater**)&inflater);
    inflater->Inflate(R::menu::call_log_options, menu);
    *allowToShow = TRUE;

    return NOERROR;
}

ECode CallLogActivity::OnPrepareOptionsMenu(
    /* [in] */ IMenu* menu,
    /* [out] */ Boolean* res)
{
    VALIDATE_NOT_NULL(res);
    AutoPtr<IMenuItem> itemDeleteAll;
    menu->FindItem(R::id::delete_all, (IMenuItem**)&itemDeleteAll);

    // If onPrepareOptionsMenu is called before fragments loaded. Don't do anything.
    if (mAllCallsFragment != NULL && itemDeleteAll != NULL) {
        AutoPtr<ICallLogAdapter> adapter = ((CCallLogFragment*)mAllCallsFragment.Get())->GetAdapter();
        Boolean isEmpty = FALSE;
        itemDeleteAll->SetVisible(adapter != NULL
                && (IAdapter::Probe(adapter)->IsEmpty(&isEmpty), !isEmpty));
    }
    *res = TRUE;

    return NOERROR;
}

// @Override
ECode CallLogActivity::OnOptionsItemSelected(
    /* [in] */ IMenuItem* item,
    /* [out] */ Boolean* res)
{
    VALIDATE_NOT_NULL(res);
    Int32 id;
    item->GetItemId(&id);
    AutoPtr<IIntent> intent;
    switch (id) {
        case Elastos::Droid::R::id::home:
            CIntent::New(this, ECLSID_CDialtactsActivity, (IIntent**)&intent);
            intent->AddFlags(IIntent::FLAG_ACTIVITY_CLEAR_TOP);
            StartActivity(intent);
            *res = TRUE;
            return NOERROR;
        case R::id::delete_all:
            AutoPtr<IFragmentManager> manager;
            GetFragmentManager((IFragmentManager**)&manager);
            ClearCallLogDialog::Show(manager);
            *res = TRUE;
            return NOERROR;
    }
    assert(0 && "TODO");
    // return AnalyticsActivity::OnOptionsItemSelected(item, res);
    return NOERROR;
}

ECode CallLogActivity::OnVoicemailStatusFetched(
    /* [in] */ ICursor* statusCursor)
{
    Boolean isFinishing = FALSE;
    if (IsFinishing(&isFinishing), isFinishing) {
        return NOERROR;
    }

    mHandler->RemoveCallbacks(mWaitForVoicemailTimeoutRunnable);
    // Update mHasActiveVoicemailProvider, which controls the number of tabs displayed.
    Int32 activeSources;
    mVoicemailStatusHelper->GetNumberActivityVoicemailSources(statusCursor, &activeSources);
    if ((activeSources > 0) != mHasActiveVoicemailProvider) {
        mHasActiveVoicemailProvider = activeSources > 0;
        assert(0 && "TODO");
        // mViewPagerAdapter->NotifyDataSetChanged();
        // mViewPagerTabs->SetViewPager(mViewPager);
        // if (mSwitchToVoicemailTab) {
        //     mViewPager->SetCurrentItem(TAB_INDEX_VOICEMAIL, FALSE);
        // }
    }
    else if (mSwitchToVoicemailTab) {
        // The voicemail tab was requested, but it does not exist because there are no
        // voicemail sources. Just fallback to the first item instead.
        assert(0 && "TODO");
        // mViewPagerTabs->SetViewPager(mViewPager);
    }

    return NOERROR;
}

ECode CallLogActivity::OnCallsFetched(
    /* [in] */ ICursor* statusCursor,
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    // Return false; did not take ownership of cursor
    *result = FALSE;
    return NOERROR;
}

} // CallLog
} // Dialer
} // Apps
} // Elastos
