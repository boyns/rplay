;GIL-3
(
(
	:type                   :menu
	:name                   file_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Sounds..." "Quit" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (nil nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (nil do_quit )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (file_menu "Quit")
		:when                   (Notify )
		:to                     (file_menu "Quit")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_quit)
		)
		(
		:from                   (file_menu "Sounds...")
		:when                   (Notify )
		:to                     (s)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
	)
)
(
	:type                   :menu
	:name                   cont_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Selected sounds" "All sounds" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (t nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (do_cont_selected do_cont_all )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (cont_menu "Selected sounds")
		:when                   (Notify )
		:to                     (cont_menu "Selected sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_cont_selected)
		)
		(
		:from                   (cont_menu "All sounds")
		:when                   (Notify )
		:to                     (cont_menu "All sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_cont_all)
		)
	)
)
(
	:type                   :menu
	:name                   pause_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Selected sounds" "All sounds" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (t nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (do_pause_selected do_pause_all )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (pause_menu "Selected sounds")
		:when                   (Notify )
		:to                     (pause_menu "Selected sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_pause_selected)
		)
		(
		:from                   (pause_menu "All sounds")
		:when                   (Notify )
		:to                     (pause_menu "All sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_pause_all)
		)
	)
)
(
	:type                   :menu
	:name                   stop_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Selected sounds" "All sounds" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (t nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (do_stop_selected do_stop_all )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (stop_menu "Selected sounds")
		:when                   (Notify )
		:to                     (stop_menu "Selected sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_stop_selected)
		)
		(
		:from                   (stop_menu "All sounds")
		:when                   (Notify )
		:to                     (stop_menu "All sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_stop_all)
		)
	)
)
(
	:type                   :menu
	:name                   delete_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Selected sounds" "All sounds" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (nil nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (playlist_delete playlist_delete_all )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (delete_menu "Selected sounds")
		:when                   (Notify )
		:to                     (delete_menu "Selected sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (playlist_delete)
		)
		(
		:from                   (delete_menu "All sounds")
		:when                   (Notify )
		:to                     (delete_menu "All sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (playlist_delete_all)
		)
	)
)
(
	:type                   :menu
	:name                   add_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Select sound" "All sounds" )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (nil nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (add_sounds add_all_sounds )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (add_menu "Select sound")
		:when                   (Notify )
		:to                     (add_menu "Select sound")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (add_sounds)
		)
		(
		:from                   (add_menu "All sounds")
		:when                   (Notify )
		:to                     (add_menu "All sounds")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (add_all_sounds)
		)
	)
)
(
	:type                   :menu
	:name                   playlistfile_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Load..." "Save..." )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (nil nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (playlistfile_load playlistfile_save )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (playlistfile_menu "Load...")
		:when                   (Notify )
		:to                     (playlistfile_menu "Load...")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (playlistfile_load)
		)
		(
		:from                   (playlistfile_menu "Save...")
		:when                   (Notify )
		:to                     (playlistfile_menu "Save...")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (playlistfile_save)
		)
		(
		:from                   (playlistfile_menu "Load...")
		:when                   (Notify )
		:to                     (f)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (playlistfile_menu "Save...")
		:when                   (Notify )
		:to                     (f)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (playlistfile_menu "Load...")
		:when                   (Notify )
		:to                     (f)
		:function_type          :user_defined
		:arg_type               (:string)
		:action                 (SetLabel "Load a Play List")
		)
		(
		:from                   (playlistfile_menu "Save...")
		:when                   (Notify )
		:to                     (f)
		:function_type          :user_defined
		:arg_type               (:string)
		:action                 (SetLabel "Save a Play List")
		)
		(
		:from                   (playlistfile_menu "Load...")
		:when                   (Notify )
		:to                     (f filedoit)
		:function_type          :user_defined
		:arg_type               (:string)
		:action                 (SetLabel "Load")
		)
		(
		:from                   (playlistfile_menu "Save...")
		:when                   (Notify )
		:to                     (f filedoit)
		:function_type          :user_defined
		:arg_type               (:string)
		:action                 (SetLabel "Save")
		)
	)
)
(
	:type                   :menu
	:name                   move_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Up" "Down" "Top" "Bottom" )
	:menu-item-label-types  (:string :string :string :string )
	:menu-item-states       (:active :active :active :active )
	:menu-item-defaults     (nil nil nil nil )
	:initial-selections     (nil nil nil nil )
	:menu-item-handlers     (move_up move_down move_top move_bottom )
	:menu-item-menus        (nil nil nil nil )
	:menu-item-colors       ("" "" "" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (move_menu "Bottom")
		:when                   (Notify )
		:to                     (move_menu "Bottom")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (move_bottom)
		)
		(
		:from                   (move_menu "Top")
		:when                   (Notify )
		:to                     (move_menu "Top")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (move_top)
		)
		(
		:from                   (move_menu "Up")
		:when                   (Notify )
		:to                     (move_menu "Up")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (move_up)
		)
		(
		:from                   (move_menu "Down")
		:when                   (Notify )
		:to                     (move_menu "Down")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (move_down)
		)
	)
)
(
	:type                   :base-window
	:name                   w
	:owner                  nil
	:width                  358
	:height                 341
	:background-color       ""
	:foreground-color       ""
	:label                  "rplaytool"
	:label-type             :string
	:initial-state          :open
	:show-footer            t
	:resizable              nil
	:icon-file              "icons/rplaytool.icon"
	:icon-label             ""
	:icon-mask-file         "icons/rplaytool_mask.icon"
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   c
	:owner                  w
	:help                   ""
	:x                      0
	:y                      0
	:width                  358
	:height                 341
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   file
	:owner                  c
	:help                   ""
	:x                      16
	:y                      8
	:width                  54
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "File"
	:label-type             :string
	:initial-state          :active
	:menu                   file_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   host
	:owner                  c
	:help                   ""
	:x                      136
	:y                      16
	:width                  212
	:height                 15
	:value-x                220
	:value-y                16
	:value-length           16
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Hostname: "
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         do_host
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (w host)
		:when                   (Notify )
		:to                     (w host)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_host)
		)
	)
)
(
	:type                   :slider
	:name                   volume
	:owner                  c
	:help                   ""
	:x                      152
	:y                      48
	:width                  197
	:height                 39
	:value-x                207
	:value-y                48
	:slider-width           100
	:ticks                  7
	:foreground-color       ""
	:label                  "Volume:"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-endboxes          t
	:show-range             nil
	:show-value             nil
	:min-value              0
	:max-value              255
	:min-value-string       ""
	:max-value-string       ""
	:min-tick-string        "0%"
	:max-tick-string        "100%"
	:initial-value          0
	:initial-state          :active
	:notify-handler         do_volume
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (w volume)
		:when                   (Notify )
		:to                     (w volume)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_volume)
		)
	)
)
(
	:type                   :scrolling-list
	:name                   spool
	:owner                  c
	:help                   ""
	:x                      16
	:y                      96
	:width                  300
	:height                 188
	:value-x                16
	:value-y                113
	:rows                   8
	:foreground-color       ""
	:label                  ""
	:title                  "Spool"
	:label-type             :string
	:layout-type            :vertical
	:read-only              t
	:multiple-selections    t
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   cont
	:owner                  c
	:help                   ""
	:x                      70
	:y                      296
	:width                  88
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Continue"
	:label-type             :string
	:initial-state          :active
	:menu                   cont_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   pause
	:owner                  c
	:help                   ""
	:x                      160
	:y                      296
	:width                  68
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Pause"
	:label-type             :string
	:initial-state          :active
	:menu                   pause_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   stop
	:owner                  c
	:help                   ""
	:x                      230
	:y                      296
	:width                  59
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Stop"
	:label-type             :string
	:initial-state          :active
	:menu                   stop_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   s
	:owner                  w
	:width                  580
	:height                 391
	:background-color       ""
	:foreground-color       ""
	:label                  "Sounds"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   controls7
	:owner                  s
	:help                   ""
	:x                      0
	:y                      0
	:width                  247
	:height                 281
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   directory
	:owner                  controls7
	:help                   ""
	:x                      8
	:y                      24
	:width                  234
	:height                 15
	:value-x                82
	:value-y                24
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         do_cd
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s directory)
		:when                   (Notify )
		:to                     (s directory)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_cd)
		)
	)
)
(
	:type                   :scrolling-list
	:name                   disklist
	:owner                  controls7
	:help                   ""
	:x                      16
	:y                      48
	:width                  200
	:height                 224
	:value-x                16
	:value-y                48
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "Disk Sounds"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_disklist
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s disklist)
		:when                   (Notify )
		:to                     (s disklist)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_disklist)
		)
	)
)
(
	:type                   :control-area
	:name                   controls9
	:owner                  s
	:help                   ""
	:x                      0
	:y                      0
	:width                  247
	:height                 281
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   soundlist
	:owner                  controls9
	:help                   ""
	:x                      14
	:y                      24
	:width                  200
	:height                 224
	:value-x                14
	:value-y                24
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "Server Sounds"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_soundlist
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s soundlist)
		:when                   (Notify )
		:to                     (s soundlist)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_soundlist)
		)
	)
)
(
	:type                   :control-area
	:name                   controls1
	:owner                  s
	:help                   ""
	:x                      247
	:y                      0
	:width                  62
	:height                 391
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   add
	:owner                  controls1
	:help                   ""
	:x                      3
	:y                      112
	:width                  56
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Add"
	:label-type             :string
	:initial-state          :inactive
	:menu                   add_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   close
	:owner                  controls1
	:help                   ""
	:x                      4
	:y                      368
	:width                  51
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Close"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s close)
		:when                   (Notify )
		:to                     (s)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Hide)
		)
	)
)
(
	:type                   :control-area
	:name                   controls4
	:owner                  s
	:help                   ""
	:x                      309
	:y                      0
	:width                  271
	:height                 257
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   playlist3
	:owner                  controls4
	:help                   ""
	:x                      18
	:y                      24
	:width                  200
	:height                 224
	:value-x                18
	:value-y                24
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "User Play List 3"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_playlist3
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s playlist3)
		:when                   (Notify )
		:to                     (s playlist3)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_playlist3)
		)
	)
)
(
	:type                   :control-area
	:name                   controls5
	:owner                  s
	:help                   ""
	:x                      309
	:y                      0
	:width                  271
	:height                 257
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   playlist4
	:owner                  controls5
	:help                   ""
	:x                      18
	:y                      24
	:width                  200
	:height                 224
	:value-x                18
	:value-y                24
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "User Play List 4"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_playlist4
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s playlist4)
		:when                   (Notify )
		:to                     (s playlist4)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_playlist4)
		)
	)
)
(
	:type                   :control-area
	:name                   controls2
	:owner                  s
	:help                   ""
	:x                      309
	:y                      0
	:width                  271
	:height                 257
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   playlist1
	:owner                  controls2
	:help                   ""
	:x                      18
	:y                      24
	:width                  200
	:height                 224
	:value-x                18
	:value-y                24
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "User Play List 1"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_playlist1
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s playlist1)
		:when                   (Notify )
		:to                     (s playlist1)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_playlist1)
		)
	)
)
(
	:type                   :control-area
	:name                   controls3
	:owner                  s
	:help                   ""
	:x                      309
	:y                      0
	:width                  271
	:height                 257
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   playlist2
	:owner                  controls3
	:help                   ""
	:x                      18
	:y                      24
	:width                  200
	:height                 224
	:value-x                18
	:value-y                24
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  "User Play List 2"
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         do_playlist2
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (s playlist2)
		:when                   (Notify )
		:to                     (s playlist2)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_playlist2)
		)
	)
)
(
	:type                   :control-area
	:name                   controls6
	:owner                  s
	:help                   ""
	:x                      309
	:y                      257
	:width                  271
	:height                 134
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   move
	:owner                  controls6
	:help                   ""
	:x                      31
	:y                      16
	:width                  64
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Move"
	:label-type             :string
	:initial-state          :inactive
	:menu                   move_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   delete
	:owner                  controls6
	:help                   ""
	:x                      107
	:y                      16
	:width                  71
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Delete"
	:label-type             :string
	:initial-state          :inactive
	:menu                   delete_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   file
	:owner                  controls6
	:help                   ""
	:x                      186
	:y                      16
	:width                  54
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "File"
	:label-type             :string
	:initial-state          :active
	:menu                   playlistfile_menu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   playit
	:owner                  controls6
	:help                   ""
	:x                      32
	:y                      48
	:width                  44
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Play"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_playit
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s playit)
		:when                   (Notify )
		:to                     (s playit)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_playit)
		)
	)
)
(
	:type                   :button
	:name                   next
	:owner                  controls6
	:help                   ""
	:x                      87
	:y                      48
	:width                  46
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Next"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_next
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s next)
		:when                   (Notify )
		:to                     (s next)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_next)
		)
	)
)
(
	:type                   :button
	:name                   prev
	:owner                  controls6
	:help                   ""
	:x                      141
	:y                      48
	:width                  45
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Prev"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_prev
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s prev)
		:when                   (Notify )
		:to                     (s prev)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_prev)
		)
	)
)
(
	:type                   :button
	:name                   stopit
	:owner                  controls6
	:help                   ""
	:x                      194
	:y                      48
	:width                  45
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Stop"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_stopit
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s stopit)
		:when                   (Notify )
		:to                     (s stopit)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_stopit)
		)
	)
)
(
	:type                   :setting
	:name                   whichlist
	:owner                  controls6
	:help                   ""
	:x                      25
	:y                      80
	:width                  209
	:height                 23
	:value-x                138
	:value-y                80
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Select play list:"
	:label-type             :string
	:notify-handler         switch_playlist
	:event-handler          nil
	:choices                ("1" "2" "3" "4" )
	:choice-label-types     (:string :string :string :string )
	:choice-colors          ("" "" "" "" )
	:initial-selections     (nil nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (s whichlist)
		:when                   (Notify )
		:to                     (s whichlist)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (switch_playlist)
		)
		(
		:from                   (s whichlist "1")
		:when                   (Notify )
		:to                     (s controls2)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (s whichlist "2")
		:when                   (Notify )
		:to                     (s controls3)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (s whichlist "3")
		:when                   (Notify )
		:to                     (s controls4)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (s whichlist "4")
		:when                   (Notify )
		:to                     (s controls5)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
	)
)
(
	:type                   :control-area
	:name                   controls8
	:owner                  s
	:help                   ""
	:x                      0
	:y                      281
	:width                  247
	:height                 110
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   sort
	:owner                  controls8
	:help                   ""
	:x                      69
	:y                      16
	:width                  42
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Sort"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_soundsort
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s sort)
		:when                   (Notify )
		:to                     (s sort)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_soundsort)
		)
	)
)
(
	:type                   :button
	:name                   reload
	:owner                  controls8
	:help                   ""
	:x                      119
	:y                      16
	:width                  59
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Reload"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         do_reload
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (s reload)
		:when                   (Notify )
		:to                     (s reload)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (do_reload)
		)
	)
)
(
	:type                   :setting
	:name                   soundsource
	:owner                  controls8
	:help                   ""
	:x                      8
	:y                      48
	:width                  224
	:height                 23
	:value-x                136
	:value-y                48
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "List sounds from:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Server" "Disk" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (t nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (s soundsource "Server")
		:when                   (Notify )
		:to                     (s controls9)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
		(
		:from                   (s soundsource "Disk")
		:when                   (Notify )
		:to                     (s controls7)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
	)
)
(
	:type                   :stack
	:name                   layered_pane1
	:owner                  s
	:members                (controls2 controls4 controls5 controls3 )
	:user-data              ()
)
(
	:type                   :stack
	:name                   layered_pane2
	:owner                  s
	:members                (controls7 controls9 )
	:user-data              ()
)
(
	:type                   :popup-window
	:name                   f
	:owner                  w
	:width                  344
	:height                 351
	:background-color       ""
	:foreground-color       ""
	:label                  ""
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   controls10
	:owner                  f
	:help                   ""
	:x                      0
	:y                      0
	:width                  344
	:height                 351
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   filedir
	:owner                  controls10
	:help                   ""
	:x                      7
	:y                      16
	:width                  330
	:height                 15
	:value-x                81
	:value-y                16
	:value-length           32
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         filedir_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (f filedir)
		:when                   (Notify )
		:to                     (f filedir)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (filedir_notify)
		)
	)
)
(
	:type                   :scrolling-list
	:name                   filelist
	:owner                  controls10
	:help                   ""
	:x                      13
	:y                      48
	:width                  300
	:height                 200
	:value-x                13
	:value-y                48
	:rows                   10
	:foreground-color       ""
	:label                  ""
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         filelist_notify
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (f filelist)
		:when                   (Notify )
		:to                     (f filelist)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (filelist_notify)
		)
	)
)
(
	:type                   :text-field
	:name                   filename
	:owner                  controls10
	:help                   ""
	:x                      26
	:y                      264
	:width                  292
	:height                 15
	:value-x                62
	:value-y                264
	:value-length           32
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "File:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         filename_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (f filename)
		:when                   (Notify )
		:to                     (f filename)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (filename_notify)
		)
	)
)
(
	:type                   :button
	:name                   filedoit
	:owner                  controls10
	:help                   ""
	:x                      147
	:y                      296
	:width                  46
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Load"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         filedoit_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (f filedoit)
		:when                   (Notify )
		:to                     (f filedoit)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (filedoit_notify)
		)
	)
)
(
	:type                   :button
	:name                   fileclose
	:owner                  controls10
	:help                   ""
	:x                      145
	:y                      323
	:width                  51
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Close"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (f fileclose)
		:when                   (Notify )
		:to                     (f)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Hide)
		)
	)
)
)
