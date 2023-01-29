#include "../nfc_i.h"

#include "flipper_application.h"
#include <fap_loader/elf_cpp/elf_hashtable.h>

void nfc_scene_mf_ultralight_read_success_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    Nfc* nfc = context;

    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(nfc->view_dispatcher, result);
    }
}

void nfc_scene_mf_ultralight_read_success_on_enter(void* context) {
    Nfc* nfc = context;

    // Setup widget view
    FuriHalNfcDevData* data = &nfc->dev->dev_data.nfc_data;
    MfUltralightData* mf_ul_data = &nfc->dev->dev_data.mf_ul_data;
    Widget* widget = nfc->widget;
    widget_add_button_element(
        widget,
        GuiButtonTypeLeft,
        "Retry",
        nfc_scene_mf_ultralight_read_success_widget_callback,
        nfc);
    widget_add_button_element(
        widget,
        GuiButtonTypeRight,
        "More",
        nfc_scene_mf_ultralight_read_success_widget_callback,
        nfc);

    FuriString* temp_str = NULL;

    // ElfApiInterface api_interface;
    FlipperApplication* app;
    app = flipper_application_alloc(nfc->dev->storage, &hashtable_api_interface);
    FlipperApplicationPreloadStatus preload_res =
        flipper_application_preload(app, "/ext/apps/nfc_parser.fap");
    FURI_LOG_I("NFC", "preload res: %d", preload_res);

    FlipperApplicationLoadStatus load_status = flipper_application_map_to_memory(app);
    FURI_LOG_I("NFC", "load statud: %d", load_status);

    FuriThread* thread = flipper_application_spawn(app, (void*)&nfc->dev->dev_data);
    furi_thread_start(thread);
    furi_thread_join(thread);
    int ret = furi_thread_get_return_code(thread);
    FURI_LOG_I("NFC", "Ret code: %d", ret);
    flipper_application_free(app);

    if(furi_string_size(nfc->dev->dev_data.parsed_data)) {
        temp_str = furi_string_alloc_set(nfc->dev->dev_data.parsed_data);
    } else {
        temp_str = furi_string_alloc_printf("\e#%s\n", nfc_mf_ul_type(mf_ul_data->type, true));
        furi_string_cat_printf(temp_str, "UID:");
        for(size_t i = 0; i < data->uid_len; i++) {
            furi_string_cat_printf(temp_str, " %02X", data->uid[i]);
        }
        furi_string_cat_printf(
            temp_str, "\nPages Read: %d/%d", mf_ul_data->data_read / 4, mf_ul_data->data_size / 4);
        if(mf_ul_data->data_read != mf_ul_data->data_size) {
            furi_string_cat_printf(temp_str, "\nPassword-protected pages!");
        }
    }
    widget_add_text_scroll_element(widget, 0, 0, 128, 52, furi_string_get_cstr(temp_str));
    furi_string_free(temp_str);

    notification_message_block(nfc->notifications, &sequence_set_green_255);

    view_dispatcher_switch_to_view(nfc->view_dispatcher, NfcViewWidget);
}

bool nfc_scene_mf_ultralight_read_success_on_event(void* context, SceneManagerEvent event) {
    Nfc* nfc = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            scene_manager_next_scene(nfc->scene_manager, NfcSceneRetryConfirm);
            consumed = true;
        } else if(event.event == GuiButtonTypeRight) {
            scene_manager_next_scene(nfc->scene_manager, NfcSceneMfUltralightMenu);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        scene_manager_next_scene(nfc->scene_manager, NfcSceneExitConfirm);
        consumed = true;
    }

    return consumed;
}

void nfc_scene_mf_ultralight_read_success_on_exit(void* context) {
    Nfc* nfc = context;

    notification_message_block(nfc->notifications, &sequence_reset_green);

    // Clean view
    widget_reset(nfc->widget);
}
