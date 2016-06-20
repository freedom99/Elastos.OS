#include "DialtactsActivity.h"

namespace Elastos {
namespace Apps {
namespace Dialer {

//================================================================
// DialtactsActivity::OptionsPopupMenu
//================================================================
DialtactsActivity::OptionsPopupMenu::OptionsPopupMenu(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::OptionsPopupMenu::constructor(
    /* [in] */ IContext* context,
    /* [in] */ IView* anchor)
{
    return PopupMenu::constructor(context, anchor);
}

ECode DialtactsActivity::OptionsPopupMenu::Show()
{
    AutoPtr<IMenu> menu;
    GetMenu((IMenu**)&menu);
    AutoPtr<IMenuItem> clearFrequents;
    menu->FindItem(R::id::menu_clear_frequents, (IMenuItem**)&clearFrequents);

    AutoPtr<ISpeedDialFragment> fragment;
    Boolean hasFrequents;
    clearFrequents->SetVisible(mHost->mListsFragment != NULL &&
            mHost->mListsFragment->GetSpeedDialFragment(
                    (ISpeedDialFragment&&)&fragment), fragment != null &&
            fragment->HasFrequents(&hasFrequents), hasFrequents);
    PopupMenu::show();
    return NOERROR;
}

//================================================================
// DialtactsActivity::LayoutOnDragListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::LayoutOnDragListener, Object, IViewOnDragListener);

DialtactsActivity::LayoutOnDragListener::LayoutOnDragListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::LayoutOnDragListener::OnDrag(
    /* [in] */ IView* v,
    /* [in] */ IDragEvent* event,
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    Int32 action;
    event->GetAction(&action);
    if (action == IDragEvent::ACTION_DRAG_LOCATION) {
        Folat x, y;
        event->GetX(&x);
        event->GetY(&y);
        mHost->mDragDropController->HandleDragHovered(v, (Int32) x, (Int32) y);
    }
    *result = TRUE;
    return NOERROR;
}

//================================================================
// DialtactsActivity::PhoneSearchQueryTextListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::PhoneSearchQueryTextListener, Object, ITextWatcher);

DialtactsActivity::PhoneSearchQueryTextListener::PhoneSearchQueryTextListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::PhoneSearchQueryTextListener::BeforeTextChanged(
    /* [in] */ ICharSequence* s,
    /* [in] */ Int32 start,
    /* [in] */ Int32 count,
    /* [in] */ Int32 after)
{
    return NOERROR;
}

ECode DialtactsActivity::PhoneSearchQueryTextListener::OnTextChanged(
    /* [in] */ ICharSequence* s,
    /* [in] */ Int32 start,
    /* [in] */ Int32 before,
    /* [in] */ Int32 count)
{
    String newText;
    s->ToString(&newText);
    if (newText.Equals(mHost->mSearchQuery)) {
        // If the query hasn't changed (perhaps due to activity being destroyed
        // and restored, or user launching the same DIAL intent twice), then there is
        // no need to do anything here.
        return NOERROR;
    }
    if (DEBUG) {
        Logger::d(TAG, "onTextChange for mSearchView called with new query: %s", newText.string());
        Logger::d(TAG, "Previous Query: %s", mHost->mSearchQuery.string());
    }
    mHost->mSearchQuery = newText;

    // Show search fragment only when the query string is changed to non-empty text.
    if (!TextUtils::IsEmpty(newText)) {
        // Call enterSearchUi only if we are switching search modes, or showing a search
        // fragment for the first time.
        Boolean sameSearchMode = (mHost->mIsDialpadShown && mHost->mInDialpadSearch) ||
                (!mHost->mIsDialpadShown && mHost->mInRegularSearch);
        if (!sameSearchMode) {
            EnterSearchUi(mIsDialpadShown, mSearchQuery);
        }
    }

    Boolean visible;
    if (mHost->mSmartDialSearchFragment != NULL &&
            mHost->mSmartDialSearchFragment->IsVisible(&visible), visible) {
        mHost->mSmartDialSearchFragment->SetQueryString(mHost->mSearchQuery, FALSE /* delaySelection */);
    }
    else if (mHost->mRegularSearchFragment != NULL &&
            mHost->mRegularSearchFragment->IsVisible(&visible), visible) {
        mHost->mRegularSearchFragment->SetQueryString(mHost->mSearchQuery, FALSE /* delaySelection */);
    }

    return NOERROR;
}

ECode DialtactsActivity::PhoneSearchQueryTextListener::AfterTextChanged(
    /* [in] */ IEditable* s)
{
    return NOERROR;
}

//================================================================
// DialtactsActivity::SearchViewOnClickListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::SearchViewOnClickListener, Object, IViewOnClickListener);

DialtactsActivity::SearchViewOnClickListener::SearchViewOnClickListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::SearchViewOnClickListener::OnClick(
    /* [in] */ IView* v)
{
    Boolean result;
    if (mHost->IsInSearchUi(&result), !result) {
        mHost->mActionBarController->OnSearchBoxTapped();
        String text;
        mHost->mSearchView->GetText(&text);
        mHost->EnterSearchUi(FALSE /* smartDialSearch */, text);
    }

    return NOERROR;
}

//================================================================
// DialtactsActivity::SearchEditTextLayoutListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::SearchEditTextLayoutListener, Object, IViewOnKeyListener);

DialtactsActivity::SearchEditTextLayoutListener::SearchEditTextLayoutListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::SearchEditTextLayoutListener::OnKey(
    /* [in] */ IView* v,
    /* [in] */ Int32 keyCode,
    /* [in] */ IKeyEvent* event,
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    Int32 action;
    String text;
    if (keyCode == IKeyEvent::KEYCODE_BACK &&
            event->GetAction(&action), action == IKeyEvent::ACTION_DOWN &&
            TextUtils::IsEmpty(mSearchView->GetText(&text), text)) {
        mHost->MaybeExitSearchUi();
    }
    *result = FALSE;
    return NOERROR;
}

//================================================================
// DialtactsActivity::MyOnBackButtonClickedListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::MyOnBackButtonClickedListener, Object,
        ISearchEditTextLayoutOnBackButtonClickedListener);

DialtactsActivity::MyOnBackButtonClickedListener::MyOnBackButtonClickedListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::MyOnBackButtonClickedListener::OnBackButtonClicked()
{
    return mHost->OnBackPressed();
}

//================================================================
// DialtactsActivity::MyOnGlobalLayoutListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::MyOnGlobalLayoutListener, Object,
        IOnGlobalLayoutListener);

DialtactsActivity::MyOnGlobalLayoutListener::MyOnGlobalLayoutListener(
    /* [in] */ DialtactsActivity* host,
    /* [in] */ IViewTreeObserver* observer)
    : mHost(host)
    , mObserver(observer)
{}

ECode DialtactsActivity::MyOnGlobalLayoutListener::OnGlobalLayout()
{
    Boolean isAlive;
    if (mObserver->IsAlive(&isAlive), !isAlive) {
        return NOERROR;
    }
    mObserver->RemoveOnGlobalLayoutListener(this);
    Int32 screenWidth;
    mHost->mParentLayout->GetWidth(&screenWidth);
    mHost->mFloatingActionButtonController->SetScreenWidth(screenWidth);
    mHost->UpdateFloatingActionButtonControllerAlignment(FALSE /* animate */);
    return NOERROR;
}

//================================================================
// DialtactsActivity::MyOnTouchListener
//================================================================
CAR_INTERFACE_IMPL(DialtactsActivity::MyOnTouchListener, Object, IViewOnTouchListener);

DialtactsActivity::MyOnTouchListener::MyOnTouchListener(
    /* [in] */ DialtactsActivity* host)
    : mHost(host)
{}

ECode DialtactsActivity::MyOnTouchListener::OnTouch(
    /* [in] */ IView* v,
    /* [in] */ IMotionEvent* event,
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    if (!mHost->mIsDialpadShown) {
        mHost->MaybeExitSearchUi();
    }
    *result = FALSE;
    return NOERROR;
}

//================================================================
// DialtactsActivity
//================================================================
const String DialtactsActivity::TAG("DialtactsActivity");
const boolean DialtactsActivity::DEBUG = Logger::IsLoggable(TAG, Logger::DEBUG);

const String DialtactsActivity::CALL_ORIGIN_DIALTACTS("com.android.dialer.DialtactsActivity");

const String DialtactsActivity::KEY_IN_REGULAR_SEARCH_UI("in_regular_search_ui");
const String DialtactsActivity::KEY_IN_DIALPAD_SEARCH_UI("in_dialpad_search_ui");
const String DialtactsActivity::KEY_SEARCH_QUERY("search_query");
const String DialtactsActivity::KEY_FIRST_LAUNCH("first_launch");
const String DialtactsActivity::KEY_IS_DIALPAD_SHOWN("is_dialpad_shown");

const String DialtactsActivity::TAG_DIALPAD_FRAGMENT("dialpad");
const String DialtactsActivity::TAG_REGULAR_SEARCH_FRAGMENT("search");
const String DialtactsActivity::TAG_SMARTDIAL_SEARCH_FRAGMENT("smartdial");
const String DialtactsActivity::TAG_FAVORITES_FRAGMENT("favorites");

const String DialtactsActivity::ACTION_TOUCH_DIALER("com.android.phone.action.TOUCH_DIALER");

const Int32 DialtactsActivity::ACTIVITY_REQUEST_CODE_VOICE_SEARCH = 1;

//TODO:
CAR_INTERFACE_IMPL_10(DialtactsActivity, TransactionSafeActivity, IDialtactsActivity,
        IViewOnClickListener, IOnDialpadQueryChangedListener, IListsFragmentHostInterface,
        ISpeedDialFragmentHostInterface, ISearchFragmentHostInterface, IOnDragDropListener,
        IOnPhoneNumberPickerActionListener, IPopupMenuOnMenuItemClickListener,
        /*IViewPagerOnPageChangeListener, */ IActionBarControllerActivityUi);

DialtactsActivity::DialtactsActivity
    : mCurrentTabPosition(0)
{
    mPhoneSearchQueryTextListener = new PhoneSearchQueryTextListener(this);
    mSearchViewOnClickListener = new SearchViewOnClickListener(this);
    mSearchEditTextLayoutListener = new SearchEditTextLayoutListener(this);
}

ECode DialtactsActivity::DispatchTouchEvent(
    /* [in] */ IMotionEvent* event,
    /* [out] */ Boolean* isConsumed)
{
    VALIDATE_NOT_NULL(isConsumed);

    Int32 action;
    ev->GetAction(&action);
    if (action == IMotionEvent::ACTION_DOWN) {
        assert(0 && "TODO");
        // TouchPointManager.getInstance().setPoint((int) ev.getRawX(), (int) ev.getRawY());
    }
    return TransactionSafeActivity::DispatchTouchEvent(ev, isConsumed);
}

ECode DialtactsActivity::OnCreate(
    /* [in] */ IBundle* savedInstanceState)
{
    TransactionSafeActivity::OnCreate(savedInstanceState);
    mFirstLaunch = TRUE;

    AutoPtr<IResources> resources;
    GetResources((IResources**)&resources);
    resources->GetDimensionPixelSize(
            R::dimen::action_bar_height_large, &mActionBarHeight);

    SetContentView(R::layout::dialtacts_activity);

    AutoPtr<IWindow> window;
    GetWindow((IWindow**)&window);
    window->SetBackgroundDrawable(NULL);

    AutoPtr<IActionBar> actionBar;
    GetActionBar((IActionBar**)&actionBar);
    actionBar->SetCustomView(R::layout::search_edittext);
    actionBar->SetDisplayShowCustomEnabled(TRUE);
    actionBar->SetBackgroundDrawable(NULL);

    AutoPtr<IView> customView;
    actionBar->GetCustomView((IView**)&customView);
    ISearchEditTextLayout* searchEditTextLayout = ISearchEditTextLayout::Probe(customView);
    CActionBarController::New(this, searchEditTextLayout,
             (IActionBarController**)&mActionBarController);

    searchEditTextLayout->SetPreImeKeyListener(mSearchEditTextLayoutListener);

    AutoPtr<IView> view;
    searchEditTextLayout->FindViewById(R::id::search_view, (IView**)&view);
    mSearchView = IEditText::Probe(view);
    mSearchView->AddTextChangedListener(mPhoneSearchQueryTextListener);
    searchEditTextLayout->FindViewById(R::id::voice_search_button, (IView**)&mVoiceSearchButton);

    view = NULL;
    searchEditTextLayout->FindViewById(R::id::search_magnifying_glass, (IView**)&view);
    view->SetOnClickListener(mSearchViewOnClickListener);
    view = NULL;
    searchEditTextLayout->FindViewById(R::id::search_box_start_search, (IView**)&view);
    view->SetOnClickListener(mSearchViewOnClickListener);
    searchEditTextLayout->SetOnBackButtonClickedListener(new MyOnBackButtonClickedListener(this));

    AutoPtr<IResources> res;
    GetResources((IResources**)&res);
    AutoPtr<IConfiguration> config;
    res->GetConfiguration((IConfiguration**)&config);
    Int32 orientation;
    config->GetOrientation(&orientation);
    mIsLandscape = orientation == IConfiguration::ORIENTATION_LANDSCAPE;

    AutoPtr<IView> floatingActionButtonContainer;
    FindViewById(R::id::floating_action_button_container,
            (IView**)&floatingActionButtonContainer);

    view = NULL;
    FindViewById(R::id::floating_action_button, (IView**)&view);
    IImageButton* floatingActionButton = IImageButton::Probe(view);
    floatingActionButton->SetOnClickListener(this);
    CFloatingActionButtonController::New(this,
            floatingActionButtonContainer, floatingActionButton,
            (IFloatingActionButtonController**)&mFloatingActionButtonController);

    view = NULL;
    searchEditTextLayout->FindViewById(R::id::dialtacts_options_menu_button, (IView**)&view);
    IImageButton* optionsMenuButton = IImageButton::Probe(view);
    optionsMenuButton->SetOnClickListener(this);
    mOverflowMenu = BuildOptionsMenu(searchEditTextLayout);
    AutoPtr<IViewOnTouchListener> listener;
    mOverflowMenu->GetDragToOpenListener((IViewOnTouchListener**)&listener);
    optionsMenuButton->SetOnTouchListener(listener);

    // Add the favorites fragment, and the dialpad fragment, but only if savedInstanceState
    // is null. Otherwise the fragment manager takes care of recreating these fragments.
    if (savedInstanceState == NULL) {
        AutoPtr<IFragmentManager> manager;
        GetFragmentManager((IFragmentManager**)&manager);
        AutoPtr<IFragmentTransaction> transaction;
        manager->BeginTransaction((IFragmentTransaction**)&transaction);
        AutoPtr<IFragment> listFragment;
        CListsFragment::New((IFragment**)&listFragment);
        transaction->Add(R::id::dialtacts_frame, listFragment, TAG_FAVORITES_FRAGMENT);
        AutoPtr<IFragment> dialpadFragment;
        CDialpadFragment::New((IFragment**)&dialpadFragment);
        transaction->Add(R::id::dialtacts_container, dialpadFragment, TAG_DIALPAD_FRAGMENT);
        transaction->Commit();
    }
    else {
         savedInstanceState->GetString(KEY_SEARCH_QUERY, &mSearchQuery);
         savedInstanceState->GetBoolean(KEY_IN_REGULAR_SEARCH_UI, &mInRegularSearch);
         savedInstanceState->GetBoolean(KEY_IN_DIALPAD_SEARCH_UI, &mInDialpadSearch);
         savedInstanceState->GetBoolean(KEY_FIRST_LAUNCH, &mFirstLaunch);
         savedInstanceState->GetBoolean(KEY_IS_DIALPAD_SHOWN, &mShowDialpadOnResume);
        mActionBarController->RestoreInstanceState(savedInstanceState);
    }

    Boolean isLayoutRtl = DialerUtils::isRtl();
    AutoPtr<IAnimationUtils> autils;
    CAnimationUtils::AcquireSingleton((IAnimationUtils**)&autils);
    if (mIsLandscape) {
        autils->LoadAnimation(this,
                isLayoutRtl ? R::anim::dialpad_slide_in_left : R::anim::dialpad_slide_in_right,
                (IAnimation**)&mSlideIn);
        autils->LoadAnimation(this,
                isLayoutRtl ? R::anim::dialpad_slide_out_left : R::anim::dialpad_slide_out_right,
                (IAnimation**)&mSlideOut);
    }
    else {
        autils->LoadAnimation(this, R::anim::dialpad_slide_in_bottom,
                (IAnimation**)&mSlideIn);
        autils->LoadAnimation(this, R::anim::dialpad_slide_out_bottom,
                (IAnimation**)&mSlideOut);
    }

    mSlideIn->SetInterpolator(IAnimUtils::EASE_IN);
    mSlideOut->SetInterpolator(IAnimUtils::EASE_OUT);

    mSlideOut->SetAnimationListener(mSlideOutListener);

    view = NULL;
    FindViewById(R::id::dialtacts_mainlayout, (IView**)&view);
    mParentLayout = IFrameLayout::Probe(view);
    mParentLayout->SetOnDragListener(new LayoutOnDragListener(this));
    AutoPtr<IViewTreeObserver> observer;
    floatingActionButtonContainer->GetViewTreeObserver((IViewTreeObserver**)&observer);
    observer->AddOnGlobalLayoutListener(new MyOnGlobalLayoutListener(this, observer));

    SetupActivityOverlay();

    AutoPtr<IDatabaseHelperManager> dbHelper;
    CDatabaseHelperManager::AcquireSingleton((IDatabaseHelperManager**)&dbHelper);
    dbHelper->GetDatabaseHelper(IContext::Probe(this),
            (IDialerDatabaseHelper**)&mDialerDatabaseHelper);
    SmartDialPrefix::InitializeNanpSettings(this);

    return NOERROR;
}

void DialtactsActivity::SetupActivityOverlay()
{
    AutoPtr<IView> activityOverlay;
    FindViewById(R::id::activity_overlay, (IView**)&activityOverlay);
    activityOverlay->SetOnTouchListener(new MyOnTouchListener(this));
}

ECode DialtactsActivity::OnResume()
{
    TransactionSafeActivity::onResume();
    if (mFirstLaunch) {
        AutoPtr<IIntent> intent;
        GetIntent((IIntent**)&intent);
        DisplayFragment(intent);
    }
    else if (!PhoneIsInUse() && mInCallDialpadUp) {
        HideDialpadFragment(FALSE, TRUE);
        mInCallDialpadUp = FALSE;
    }
    else if (mShowDialpadOnResume) {
        ShowDialpadFragment(FALSE);
        mShowDialpadOnResume = FALSE;
    }
    mFirstLaunch = FALSE;
    PrepareVoiceSearchButton();
    mDialerDatabaseHelper->StartSmartDialUpdateThread();
    UpdateFloatingActionButtonControllerAlignment(FALSE /* animate */);
    return NOERROR;
}

ECode DialtactsActivity::OnPause()
{
    if (mClearSearchOnPause) {
        HideDialpadAndSearchUi();
        mClearSearchOnPause = FALSE;
    }
    return TransactionSafeActivity::OnPause();
}

ECode DialtactsActivity::OnSaveInstanceState(
    /* [in] */ IBundle* outState)
{
    TransactionSafeActivity::OnSaveInstanceState(outState);
    outState->PutString(KEY_SEARCH_QUERY, mSearchQuery);
    outState->PutBoolean(KEY_IN_REGULAR_SEARCH_UI, mInRegularSearch);
    outState->PutBoolean(KEY_IN_DIALPAD_SEARCH_UI, mInDialpadSearch);
    outState->PutBoolean(KEY_FIRST_LAUNCH, mFirstLaunch);
    outState->PutBoolean(KEY_IS_DIALPAD_SHOWN, mIsDialpadShown);
    mActionBarController->SaveInstanceState(outState);
    return NOERROR;
}

ECode DialtactsActivity::OnAttachFragment(
    /* [in] */ IFragment* fragment)
{
    if (IDialpadFragment::Probe(fragment) != NULL) {
        mDialpadFragment = IDialpadFragment::Probe(fragment);
        if (!mShowDialpadOnResume) {
            AutoPtr<IFragmentManager> manager;
            GetFragmentManager((IFragmentManager**)&manager);
            AutoPtr<IFragmentTransaction> transaction;
            manager->BeginTransaction((IFragmentTransaction**)&transaction);
            transaction->Hide(mDialpadFragment);
            transaction->Commit();
        }
    }
    else if (ISmartDialSearchFragment::Probe(fragment) != NULL) {
        mSmartDialSearchFragment = ISmartDialSearchFragment::Probe(fragment);
        mSmartDialSearchFragment->SetOnPhoneNumberPickerActionListener(this);
    }
    else if (ISearchFragment::Probe(fragment) != NULL) {
        mRegularSearchFragment = IRegularSearchFragment::Probe(fragment);
        mRegularSearchFragment->SetOnPhoneNumberPickerActionListener(this);
    }
    else if (IListsFragment::Probe(fragment) != NULL) {
        mListsFragment = IListsFragment::Probe(fragment);
        mListsFragment->AddOnPageChangeListener(this);
    }
    return NOERROR;
}

void DialtactsActivity::HandleMenuSettings()
{
    AutoPtr<IIntent> intent;
    CIntent::New(IContext::Probe(this), ECLSID_CDialerSettingsActivity, (IIntent**)&intent);
    StartActivity(intent);
}

ECode DialtactsActivity::OnClick(
    /* [in] */ IView* view)
{
    Int32 id;
    view->GetId(&id);
    switch (id) {
        case R::id::floating_action_button:
            if (!mIsDialpadShown) {
                mInCallDialpadUp = FALSE;
                ShowDialpadFragment(TRUE);
            }
            break;
        case R::id::voice_search_button:
            // try {
            AutoPtr<IIntent> intent;
            CIntent::New(IRecognizerIntent::ACTION_RECOGNIZE_SPEECH, (IIntent**)&intent);
            ECode ec = StartActivityForResult(intent, ACTIVITY_REQUEST_CODE_VOICE_SEARCH);
            if (ec == (ECode)E_ACTIVITY_NOT_FOUND_EXCEPTION) {
                AutoPtr<IToastHelper> toastHelper;
                CToastHelper::AcquireSingleton((IToastHelper**)&toastHelper);
                AutoPtr<IToast> toast;
                toastHelper->MakeText(ECLSID_DialtactsActivity,
                        R::string::voice_search_not_available,
                        IToast::LENGTH_SHORT, (IToast**)&toast);
                toast->Show();
            }
            // } catch (ActivityNotFoundException e) {
            //     Toast.makeText(DialtactsActivity.this, R.string.voice_search_not_available,
            //             Toast.LENGTH_SHORT).show();
            // }
            break;
        case R::id::dialtacts_options_menu_button:
            mOverflowMenu->Show();
            break;
        default: {
            String str;
            view->ToString(&str);
            Logger::Wtf(TAG, "Unexpected onClick event from %s", str.string());
            break;
        }
    }
    return NOERROR;
}

ECode DialtactsActivity::OnMenuItemClick(
    /* [in] */ IVMenuItem* item,
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);

    Int32 id;
    item->GetItemId(&id);
    switch (id) {
        case R::id::menu_history:
            ShowCallHistory();
            break;
        case R::id::menu_add_contact:
            // try {
            AutoPtr<IContactsContractContacts> contacts;
            CContactsContractContacts::AcquireSingleton((IContactsContractContacts**)&contacts);
            AutoPtr<IUri> uri;
            contacts->GetCONTENT_URI((IUri**)&uri);
            AutoPtr<IIntent> intent;
            CIntent::New(IIntent::ACTION_INSERT, uri, (IIntent**)&intent);
            ECode ec = StartActivity(intent);
            if (ec == (ECode)E_ACTIVITY_NOT_FOUND_EXCEPTION) {
                AutoPtr<IToastHelper> toastHelper;
                CToastHelper::AcquireSingleton((IToastHelper**)&toastHelper);
                AutoPtr<IToast> toast;
                toastHelper->MakeText(this,
                        R::string::add_contact_not_available,
                        IToast::LENGTH_SHORT, (IToast**)&toast);
                toast->Show();
            }
            // } catch (ActivityNotFoundException e) {
            //     Toast toast = Toast.makeText(this,
            //             R.string.add_contact_not_available,
            //             Toast.LENGTH_SHORT);
            //     toast.show();
            // }
            break;
        case R::id::menu_import_export:
            // We hard-code the "contactsAreAvailable" argument because doing it properly would
            // involve querying a {@link ProviderStatusLoader}, which we don't want to do right
            // now in Dialtacts for (potential) performance reasons. Compare with how it is
            // done in {@link PeopleActivity}.
            AutoPtr<IFragmentManager> manager;
            GetFragmentManager((IFragmentManager**)&manager);
            assert(0 && "TODO");
            // ImportExportDialogFragment.show(manager, TRUE,
            //         ECLSID_DialtactsActivity);
            *result = TRUE;
            return NOERROR;
        case R::id::menu_clear_frequents:
            AutoPtr<IFragmentManager> manager;
            GetFragmentManager((IFragmentManager**)&manager);
            assert(0 && "TODO");
            // ClearFrequentsDialog.show(manager);
            *result = TRUE;
            return NOERROR;
        case R::id::menu_call_settings:
            HandleMenuSettings();
            *result = TRUE;
            return NOERROR;
    }
    *result = FALSE;
    return NOERROR;
}

ECode DialtactsActivity::OnActivityResult(
    /* [in] */ Int32 requestCode,
    /* [in] */ Int32 resultCode,
    /* [in] */ IIntent *data)
{
    if (requestCode == ACTIVITY_REQUEST_CODE_VOICE_SEARCH) {
        if (resultCode == RESULT_OK) {
            AutoPtr<IArrayList> matches;
            data->GetStringArrayListExtra(
                    IRecognizerIntent::EXTRA_RESULTS, (IArrayList**)&matches);
            Int32 size;
            if (matches->GetSize(&size), size > 0) {
                AutoPtr<IInterface> match;
                matches->Get(0, (IInterface**)&match);
                mSearchView->SetText(CoreUtils::Unbox(match));
            }
            else {
                Logger::E(TAG, "Voice search - nothing heard");
            }
        }
        else {
            Logger::E(TAG, "Voice search failed");
        }
    }
    return TransactionSafeActivity::OnActivityResult(requestCode, resultCode, data);
}

void DialtactsActivity::ShowDialpadFragment(
    /* [in] */ Boolean animate)
{
    if (mIsDialpadShown) {
        return;
    }
    mIsDialpadShown = TRUE;
    mDialpadFragment->SetAnimate(animate);
    mDialpadFragment->SendScreenView();

    AutoPtr<IFragmentManager> manager;
    GetFragmentManager((IFragmentManager**)&manager);
    AutoPtr<IFragmentTransaction> ft;
    manager->BeginTransaction((IFragmentTransaction**)&ft);
    ft->Show(mDialpadFragment);
    ft->Commit();

    if (animate) {
        mFloatingActionButtonController->ScaleOut();
    }
    else {
        mFloatingActionButtonController->SetVisible(FALSE);
    }
    mActionBarController->OnDialpadUp();

    if (!IsInSearchUi()) {
        EnterSearchUi(TRUE /* isSmartDial */, mSearchQuery);
    }
}

ECode DialtactsActivity::OnDialpadShown()
{
    Boolean result;
    if (mDialpadFragment->GetAnimate(&result), result) {
        AutoPtr<IView> view;
        mDialpadFragment->GetView((IView**)&view);
        view->StartAnimation(mSlideIn);
    }
    else {
        mDialpadFragment->SetYFraction(0);
    }

    UpdateSearchFragmentPosition();
    return NOERROR;
}

ECode DialtactsActivity::HideDialpadFragment(
    /* [in] */ Boolean animate,
    /* [in] */ Boolean clearDialpad)
{
    if (mDialpadFragment == NULL) {
        return NOERROR;
    }
    if (clearDialpad) {
        mDialpadFragment->ClearDialpad();
    }
    if (!mIsDialpadShown) {
        return NOERROR;
    }
    mIsDialpadShown = FALSE;
    mDialpadFragment->SetAnimate(animate);

    UpdateSearchFragmentPosition();

    UpdateFloatingActionButtonControllerAlignment(animate);
    if (animate) {
        AutoPtr<IView> view;
        mDialpadFragment->GetView((IView**)&view);
        view->StartAnimation(mSlideOut);
    }
    else {
        CommitDialpadFragmentHide();
    }

    mActionBarController->OnDialpadDown();

    if (IsInSearchUi()) {
        if (TextUtils::IsEmpty(mSearchQuery)) {
            ExitSearchUi();
        }
    }
    return NOERROR;
}

void DialtactsActivity::CommitDialpadFragmentHide()
{
    AutoPtr<IFragmentManager> manager;
    GetFragmentManager((IFragmentManager**)&manager);
    AutoPtr<IFragmentTransaction> ft;
    manager->BeginTransaction((IFragmentTransaction**)&ft);
    ft->Hide(mDialpadFragment);
    ft->Commit();

    mFloatingActionButtonController->ScaleIn(IAnimUtils::NO_DELAY);
}

void DialtactsActivity::UpdateSearchFragmentPosition()
{
    AutoPtr<ISearchFragment> fragment;
    Boolean isVisible;
    if (mSmartDialSearchFragment != NULL &&
            mSmartDialSearchFragment->IsVisible(&isVisible), isVisible) {
        fragment = mSmartDialSearchFragment;
    }
    else if (mRegularSearchFragment != NULL &&
            mRegularSearchFragment->IsVisible(&isVisible), isVisible) {
        fragment = mRegularSearchFragment;
    }
    if (fragment != NULL && fragment->IsVisible(&isVisible), isVisible) {
        fragment->UpdatePosition(TRUE /* animate */);
    }
}

ECode DialtactsActivity::IsInSearchUi(
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    *result = mInDialpadSearch || mInRegularSearch;
    return NOERROR;
}

ECode DialtactsActivity::HasSearchQuery(
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    *result = !TextUtils::IsEmpty(mSearchQuery);
    return NOERROR;
}

ECode DialtactsActivity::ShouldShowActionBar(
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    return mListsFragment->ShouldShowActionBar(result);
}

void DialtactsActivity::SetNotInSearchUi()
{
    mInDialpadSearch = FALSE;
    mInRegularSearch = FALSE;
}

void DialtactsActivity::HideDialpadAndSearchUi()
{
    if (mIsDialpadShown) {
        HideDialpadFragment(FALSE, TRUE);
    }
    else {
        ExitSearchUi();
    }
}

void DialtactsActivity::PrepareVoiceSearchButton()
{
    AutoPtr<IIntent> voiceIntent;
    CIntent::New(IRecognizerIntent::ACTION_RECOGNIZE_SPEECH, (IIntent**)&voiceIntent);
    if (CanIntentBeHandled(voiceIntent)) {
        mVoiceSearchButton->SetVisibility(IView::VISIBLE);
        mVoiceSearchButton->SetOnClickListener(this);
    }
    else {
        mVoiceSearchButton->SetVisibility(IView::GONE);
    }
}

AutoPtr<OptionsPopupMenu> DialtactsActivity::BuildOptionsMenu(
    /* [in] */ IView* invoker)
{
    AutoPtr<OptionsPopupMenu> popupMenu = new OptionsPopupMenu(this);
    popupMenu->constructor(IContext::Probe(this), invoker);
    popupMenu->Inflate(R::menu::dialtacts_options);
    AutoPtr<IMenu> menu;
    popupMenu->GetMenu((IMenu**)&menu);
    popupMenu->SetOnMenuItemClickListener(this);
    return popupMenu;
}

ECode DialtactsActivity::OnCreateOptionsMenu(
    /* [in] */ IMenu* menu,
    /* [out] */ Boolean* allowToShow)
{
    VALIDATE_NOT_NULL(allowToShow);
    if (mPendingSearchViewQuery != NULL) {
        mSearchView->SetText(mPendingSearchViewQuery);
        mPendingSearchViewQuery = NULL;
    }
    mActionBarController->RestoreActionBarOffset();
    *allowToShow = FALSE;
    return NOERROR;
}

Boolean DialtactsActivity::IsSendKeyWhileInCall(
    /* [in] */ IIntent* intent)
{
    // If there is a call in progress and the user launched the dialer by hitting the call
    // button, go straight to the in-call screen.
    String action;
    intent->GetAction(&action);
    Boolean callKey = IIntent::ACTION_CALL_BUTTON.Equals(action);

    if (callKey) {
        GetTelecomManager()->ShowInCallScreen(FALSE);
        return TRUE;
    }

    return FALSE;
}

void DialtactsActivity::DisplayFragment(
    /* [in] */ IIntent* intent)
{
    // If we got here by hitting send and we're in call forward along to the in-call activity
    if (IsSendKeyWhileInCall(intent)) {
        Finish();
        return;
    }

    if (mDialpadFragment != NULL) {
        Boolean phoneIsInUse = PhoneIsInUse();
        AutoPtr<IUri> uri;
        if (phoneIsInUse || (intent->GetData(&uri), uri !=  NULL && IsDialIntent(intent))) {
            mDialpadFragment->SetStartedFromNewIntent(TRUE);
            Boolean isVisible;
            if (phoneIsInUse && mDialpadFragment->IsVisible(&isVisible), !isVisible) {
                mInCallDialpadUp = TRUE;
            }
            ShowDialpadFragment(FALSE);
        }
    }
}

ECode DialtactsActivity::OnNewIntent(
    /* [in] */ IIntent *intent)
{
    SetIntent(newIntent);
    DisplayFragment(newIntent);

    InvalidateOptionsMenu();
    return NOERROR;
}

Boolean DialtactsActivity::IsDialIntent(
    /* [in] */ IIntent* intent)
{
    String action;
    intent->GetAction(&action);
    if (IIntent::ACTION_DIAL.Equals(action) || ACTION_TOUCH_DIALER.Equals(action)) {
        return TRUE;
    }
    if (IIntent::ACTION_VIEW.Equals(action)) {
        AutoPtr<IUri> data;
        intent->GetData((IUri**)&data);
        String scheme;
        if (data != NULL &&
            data->GetScheme(&scheme), IPhoneAccount::SCHEME_TEL.Equals(scheme)) {
            return TRUE;
        }
    }
    return FALSE;
}

ECode DialtactsActivity::GetCallOrigin(
    /* [out] */ String* origin)
{
    VALIDATE_NOT_NULL(origin);
    AutoPtr<IIntent> intent;
    GetIntent((IIntent**)&intent);
    *origin = !IsDialIntent(intent) ? CALL_ORIGIN_DIALTACTS : NULL;
    return NOERROR;
}

void DialtactsActivity::EnterSearchUi(
    /* [in] */ Boolean smartDialSearch,
    /* [in] */ const String& query)
{
    AutoPtr<IFragmentManager> manager;
    GetFragmentManager((IFragmentManager**)&manager);
    Boolean isDestroyed;
    if (manager->IsDestroyed(&isDestroyed), isDestroyed) {
        // Weird race condition where fragment is doing work after the activity is destroyed
        // due to talkback being on (b/10209937). Just return since we can't do any
        // constructive here.
        return;
    }

    if (DEBUG) {
        Logger::D(TAG, "Entering search UI - smart dial %d", smartDialSearch);
    }

    AutoPtr<IFragmentTransaction> transaction;
    manager->BeginTransaction((IFragmentTransaction**)&transaction);
    if (mInDialpadSearch && mSmartDialSearchFragment != NULL) {
        transaction->Remove(mSmartDialSearchFragment);
    }
    else if (mInRegularSearch && mRegularSearchFragment != NULL) {
        transaction->Remove(mRegularSearchFragment);
    }

    String tag;
    if (smartDialSearch) {
        tag = TAG_SMARTDIAL_SEARCH_FRAGMENT;
    } else {
        tag = TAG_REGULAR_SEARCH_FRAGMENT;
    }
    mInDialpadSearch = smartDialSearch;
    mInRegularSearch = !smartDialSearch;

    AutoPtr<IFragment> fragment;
    manager->FindFragmentByTag(tag, (IFragment**)&fragment);
    transaction->SetCustomAnimations(Elastos::R::animator::fade_in, 0);
    if (fragment == NULL) {
        if (smartDialSearch) {
            CSmartDialSearchFragment::New((IFragment**)&fragment);
        }
        else {
            CRegularSearchFragment::New((IFragment**)&fragment);
        }
        transaction->Add(R::id::dialtacts_frame, fragment, tag);
    }
    else {
        transaction->Show(fragment);
    }
    // DialtactsActivity will provide the options menu
    fragment->SetHasOptionsMenu(FALSE);
    fragment->SetShowEmptyListForNullQuery(TRUE);
    fragment->SetQueryString(query, FALSE /* delaySelection */);
    transaction->Commit();

    AutoPtr<IView> view;
    mListsFragment->GetView((IView**)&view);
    AutoPtr<IViewPropertyAnimator> animator;
    view->Animate((IViewPropertyAnimator**)&animator);
    animator->Alpha(0);
    animator->WithLayer();
}

void DialtactsActivity::ExitSearchUi()
{
    // See related bug in enterSearchUI();
    AutoPtr<IFragmentManager> manager;
    GetFragmentManager((IFragmentManager**)&manager);
    Boolean isDestroyed;
    if (manager->IsDestroyed(&isDestroyed), isDestroyed) {
        return;
    }

    mSearchView->SetText(NULL);
    mDialpadFragment->ClearDialpad();
    SetNotInSearchUi();

    AutoPtr<IFragmentTransaction> transaction;
    manager->BeginTransaction((IFragmentTransaction**)&transaction);
    if (mSmartDialSearchFragment != NULL) {
        transaction->Remove(mSmartDialSearchFragment);
    }
    if (mRegularSearchFragment != NULL) {
        transaction->Remove(mRegularSearchFragment);
    }
    transaction->Commit();

    AutoPtr<IView> view;
    mListsFragment->GetView((IView**)&view);
    AutoPtr<IViewPropertyAnimator> animator;
    view->Animate((IViewPropertyAnimator**)&animator);
    animator->Alpha(1);
    animator->WithLayer();
    mActionBarController->OnSearchUiExited();
}

AutoPtr<IIntent> DialtactsActivity::GetCallSettingsIntent()
{
    AutoPtr<IIntent> intent;
    CIntent::New(ITelecomManager::ACTION_SHOW_CALL_SETTINGS, (IIntent**)&intent);
    intent->SetFlags(IIntent::FLAG_ACTIVITY_CLEAR_TOP);
    return intent;
}

ECode DialtactsActivity::OnBackPressed()
{
    if (mIsDialpadShown) {
        Boolean isVisible;
        AutoPtr<IInterface> adapter;
        Int32 count;
        if (TextUtils::IsEmpty(mSearchQuery) ||
                (mSmartDialSearchFragment != NULL &&
                        mSmartDialSearchFragment->IsVisible(&isVisible), isVisible
                        && IContactEntryListFragment::Probe(mSmartDialSearchFragment)
                                ->GetAdapter((IInterface**)&adapter),
                                IContactEntryListAdapter::Probe(adapter)->GetCount(&count),
                                count == 0)) {
            ExitSearchUi();
        }
        HideDialpadFragment(TRUE, FALSE);
    }
    else if (IsInSearchUi()) {
        ExitSearchUi();
        DialerUtils::HideInputMethod(mParentLayout);
    }
    else {
        TransactionSafeActivity::OnBackPressed();
    }
    return NOERROR;
}

Boolean DialtactsActivity::MaybeExitSearchUi()
{
    if (IsInSearchUi() && TextUtils::IsEmpty(mSearchQuery)) {
        ExitSearchUi();
        DialerUtils::HideInputMethod(mParentLayout);
        return TRUE;
    }
    return FALSE;
}

ECode DialtactsActivity::OnDialpadQueryChanged(
    /* [in] */ const String& query)
{
    if (mSmartDialSearchFragment != NULL) {
        mSmartDialSearchFragment->SetAddToContactNumber(query);
    }
    String normalizedQuery = SmartDialNameMatcher::NormalizeNumber(query,
            ISmartDialNameMatcher::LATIN_SMART_DIAL_MAP);

    String text;
    mSearchView->GetText(&text);
    if (!TextUtils::Equals(text, normalizedQuery)) {
        if (DEBUG) {
            Logger::D(TAG, "onDialpadQueryChanged - new query: %s", query.string());
        }
        Boolean isVisible;
        if (mDialpadFragment == NULL ||
                mDialpadFragment->IsVisible(&isVisible), 1isVisible) {
            // This callback can happen if the dialpad fragment is recreated because of
            // activity destruction. In that case, don't update the search view because
            // that would bring the user back to the search fragment regardless of the
            // previous state of the application. Instead, just return here and let the
            // fragment manager correctly figure out whatever fragment was last displayed.
            if (!TextUtils::IsEmpty(normalizedQuery)) {
                mPendingSearchViewQuery = normalizedQuery;
            }
            return NOERROR;
        }
        mSearchView->SetText(normalizedQuery);
    }
    return NOERROR;
}

ECode DialtactsActivity::OnListFragmentScrollStateChange(
    /* [in] */ Int32 scrollState)
{
    if (scrollState == IAbsListViewOnScrollListener::SCROLL_STATE_TOUCH_SCROLL) {
        HideDialpadFragment(TRUE, FALSE);
        DialerUtils::HideInputMethod(mParentLayout);
    }
    return NOERROR;
}

ECode DialtactsActivity::OnListFragmentScroll(
    /* [in] */ Int32 firstVisibleItem,
    /* [in] */ Int32 visibleItemCount,
    /* [in] */ Int32 totalItemCount)
{
    // TODO: No-op for now. This should eventually show/hide the actionBar based on
    // interactions with the ListsFragments.
    return NOERROR;
}

AutoPtr<IIntent> DialtactsActivity::GetAddNumberToContactIntent(
    /* [in] */ ICharSequence* text)
{
    AutoPtr<IIntent> intent;
    CIntent::New(IIntent::ACTION_INSERT_OR_EDIT, (IIntent**)&intent);
    intent->PutExtra(IContactsContractIntentsInsert::PHONE, text);
    intent->SetType(IContacts::CONTENT_ITEM_TYPE);
    return intent;
}

Boolean DialtactsActivity::CanIntentBeHandled(
    /* [in] */ IIntent* intent)
{
    AutoPtr<IPackageManager> packageManager;
    GetPackageManager((IPackageManager**)&packageManager);
    AutoPtr<IList> resolveInfo;
    packageManager->QueryIntentActivities(intent,
            IPackageManager::MATCH_DEFAULT_ONLY, (IList**)&resolveInfo);
    Int32 size;
    return resolveInfo != NULL && resolveInfo->GetSize(&size), size > 0;
}

ECode DialtactsActivity::ShowCallHistory()
{
    // Use explicit CallLogActivity intent instead of ACTION_VIEW +
    // CONTENT_TYPE, so that we always open our call log from our dialer
    AutoPtr<IIntent> intent;
    CIntent::New(IContext::Probe(this), ECLSID_CallLogActivity, (IIntent**)&intent);
    StartActivity(intent);
    return NOERROR;
}

ECode DialtactsActivity::OnDragStarted(
    /* [in] */ Int32 x,
    /* [in] */ Int32 y,
    /* [in] */ IPhoneFavoriteSquareTileView* view)
{
    Boolean isPaneOpen;
    if (mListsFragment->IsPaneOpen(&isPaneOpen), isPaneOpen) {
        mActionBarController->SetAlpha(IListsFragment::REMOVE_VIEW_SHOWN_ALPHA);
    }
    mListsFragment->ShowRemoveView(TRUE);
    return NOERROR;
}

ECode DialtactsActivity::OnDragHovered(
    /* [in] */ Int32 x,
    /* [in] */ Int32 y,
    /* [in] */ IPhoneFavoriteSquareTileView* view)
{
    return NOERROR;
}

ECode DialtactsActivity::OnDragFinished(
    /* [in] */ Int32 x,
    /* [in] */ Int32 y)
{
    Boolean isPaneOpen;
    if (mListsFragment->IsPaneOpen(&isPaneOpen), isPaneOpen) {
        mActionBarController->SetAlpha(IListsFragment::REMOVE_VIEW_HIDDEN_ALPHA);
    }
    mListsFragment->ShowRemoveView(FALSE);
    return NOERROR;
}

ECode DialtactsActivity::OnDroppedOnRemove()
{
    return NOERROR;
}

ECode DialtactsActivity::SetDragDropController(
    /* [in] */ IDragDropController* dragController)
{
    mDragDropController = dragController;
    AutoPtr<IRemoveView> removeView;
    mListsFragment->GetRemoveView((IRemoveView**)&removeView);
    removeView->SetDragDropController(dragController);
    return NOERROR;
}

ECode DialtactsActivity::OnPickPhoneNumberAction(
    /* [in] */ IUri* dataUri)
{
    // Specify call-origin so that users will see the previous tab instead of
    // CallLog screen (search UI will be automatically exited).
    String origin;
    GetCallOrigin(&origin);
    PhoneNumberInteraction::StartInteractionForPhoneCall(
            DialtactsActivity.this, dataUri, origin);
    mClearSearchOnPause = TRUE;
    return NOERROR;
}

ECode DialtactsActivity::OnCallNumberDirectly(
    /* [in] */ const String& phoneNumber)
{
    return OnCallNumberDirectly(phoneNumber, FALSE /* isVideoCall */);
}

ECode DialtactsActivity::OnCallNumberDirectly(
    /* [in] */ const String& phoneNumber,
    /* [in] */ Boolean isVideoCall)
{
    String origin;
    GetCallOrigin(&origin);
    AutoPtr<IIntent> intent = isVideoCall ?
            CallUtil::GetVideoCallIntent(phoneNumber, origin) :
            CallUtil::GetCallIntent(phoneNumber, origin);
    DialerUtils::StartActivityWithErrorToast(this, intent);
    mClearSearchOnPause = TRUE;
    return NOERROR;
}

ECode DialtactsActivity::OnShortcutIntentCreated(
    /* [in] */ IIntent* intent)
{
    String str;
    intent->ToString(&str);
    Logger::W(TAG, "Unsupported intent has come (%s). Ignoring.", str.string());
    return NOERROR;
}

ECode DialtactsActivity::OnHomeInActionBarSelected()
{
    ExitSearchUi();
    return NOERROR;
}

ECode DialtactsActivity::OnPageScrolled(
    /* [in] */ Int32 position,
    /* [in] */ Float positionOffset,
    /* [in] */ Int32 positionOffsetPixels)
{
    mListsFragment->GetRtlPosition(position, &position);
    // Only scroll the button when the first tab is selected. The button should scroll from
    // the middle to right position only on the transition from the first tab to the second
    // tab.
    // If the app is in RTL mode, we need to check against the second tab, rather than the
    // first. This is because if we are scrolling between the first and second tabs, the
    // viewpager will report that the starting tab position is 1 rather than 0, due to the
    // reversal of the order of the tabs.
    Boolean isLayoutRtl = DialerUtils::IsRtl();
    Boolean shouldScrollButton = position == (isLayoutRtl
            ? IListsFragment::TAB_INDEX_RECENTS : IListsFragment::TAB_INDEX_SPEED_DIAL);
    if (shouldScrollButton && !mIsLandscape) {
        mFloatingActionButtonController->OnPageScrolled(
                isLayoutRtl ? 1 - positionOffset : positionOffset);
    }
    else if (position != IListsFragment::TAB_INDEX_SPEED_DIAL) {
        mFloatingActionButtonController->OnPageScrolled(1);
    }
    return NOERROR;
}

ECode DialtactsActivity::OnPageSelected(
    /* [in] */ Int32 position)
{
    mListsFragment->GetRtlPosition(position, &position);
    mCurrentTabPosition = position;
    return NOERROR;
}

ECode DialtactsActivity::OnPageScrollStateChanged(
    /* [in] */ Int32 state)
{
    return NOERROR;
}

AutoPtr<ITelephonyManager> DialtactsActivity::GetTelephonyManager()
{
    AutoPtr<IInterface> service;
    GetSystemService(IContext::TELEPHONY_SERVICE, (IInterface**)&service);
    return ITelephonyManager::Probe(service);
}

AutoPtr<ITelecomManager> DialtactsActivity::GetTelecomManager()
{
    AutoPtr<IInterface> service;
    GetSystemService(IContext::TELECOM_SERVICE, (IInterface**)&service);
    return ITelecomManager::Probe(service);
}

ECode DialtactsActivity::IsActionBarShowing(
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    mActionBarController->IsActionBarShowing(result);
    return NOERROR;
}

ECode DialtactsActivity::IsDialpadShown(
    /* [out] */ Boolean* result)
{
    VALIDATE_NOT_NULL(result);
    *result = mIsDialpadShown;
    return NOERROR;
}

ECode DialtactsActivity::GetActionBarHideOffset(
    /* [out] */ Int32* offset)
{
    VALIDATE_NOT_NULL(offset);
    AutoPtr<IActionBar> actionBar;
    GetActionBar((IActionBar**)&actionBar);
    actionBar->GetHideOffset(offset);
    return NOERROR;
}

ECode DialtactsActivity::GetActionBarHeight(
    /* [out] */ Int32* height)
{
    VALIDATE_NOT_NULL(offset);
    *height = mActionBarHeight;
    return NOERROR;
}

ECode DialtactsActivity::SetActionBarHideOffset(
    /* [in] */ Int32 hideOffset)
{
    return mActionBarController->SetHideOffset(hideOffset);
}

void DialtactsActivity::UpdateFloatingActionButtonControllerAlignment(
    /* [in] */ Boolean animate)
{
    Int32 align = (!mIsLandscape && mCurrentTabPosition == IListsFragment::TAB_INDEX_SPEED_DIAL) ?
            IFloatingActionButtonController::ALIGN_MIDDLE :
                    IFloatingActionButtonController::ALIGN_END;
    mFloatingActionButtonController->Align(align, 0 /* offsetX */, 0 /* offsetY */, animate);
}

} // Dialer
} // Apps
} // Elastos
