    /* for localization */
    var Bundle = srGetStrBundle("chrome://wallet/locale/WalletPreview.properties");
    var heading = Bundle.GetStringFromName("heading");
    var bypass = Bundle.GetStringFromName("bypass");
    var okCmdLabel = Bundle.GetStringFromName("okCmdLabel");
    var cancelCmdLabel = Bundle.GetStringFromName("cancelCmdLabel");

    /* for xpconnect */
    var walletpreview =
      Components.classes
        ["component://netscape/walletpreview/walletpreview-world"].createInstance();
    walletpreview = walletpreview.QueryInterface(Components.interfaces.nsIWalletPreview);

    function DoGetPrefillList()
    {
      return walletpreview.GetPrefillValue();
    }

    function DoSave(value)
    {
      walletpreview.SetValue(value, window);
    }
    /* end of xpconnect stuff */

    index_frame = 0;
    title_frame = 1;
    list_frame = 2;
    button_frame = 3;
    var prefillList = [];

    function loadFillins(){
      top.frames[title_frame].document.open();
      top.frames[title_frame].document.write
        ("&nbsp;" + heading);
      top.frames[title_frame].document.close();

      top.frames[list_frame].document.open();
      top.frames[list_frame].document.write(
        "<form name='fSelectFillin'>" +
          "<br/>" +
          "<table border='0'>" +
            "<tr>" +
              "<td>" +
                "<br/>" 
      )
      var count;
      for (i=1; !(i>=prefillList.length-2); i+=3) {
        if(prefillList[i] != 0) {
          count = prefillList[i];

/* the right way to do it
 *        top.frames[list_frame].document.write(
 *              "<tr>" +
 *                "<td>" + prefillList[i+1] + ":  </td>" +
 *                "<td>" +
 *                  "<select>" 
 *        )
 *        count--;
 *      }
 *      top.frames[list_frame].document.write(
 *                    "<option VALUE='"+prefillList[i+1]+"'>" +
 *                      prefillList[i+2] +
 *                    "</option>" 
 *      )
 *
 * but because of bug 18479 we have to hack things by putting the
 * <select> and the first <option> into a single write statement as follows:
 */

          top.frames[list_frame].document.write(
                "<tr>" +
                  "<td>" + prefillList[i+1] + ":  </td>" +
                  "<td>" +
                    "<select>" +
                      "<option VALUE='"+prefillList[i+1]+"'>" +
                        prefillList[i+2] +
                      "</option>" 
          )
          count--;
        } else {
          top.frames[list_frame].document.write(
                      "<option VALUE='"+prefillList[i+1]+"'>" +
                        prefillList[i+2] +
                      "</option>" 
          )
/* end of hack */

        }
        if(count == 0) {
          top.frames[list_frame].document.write(
                      "<option VALUE='"+prefillList[i+1]+"'>&lt;do not prefill&gt;</option>" +
                    "</select><br/>" +
                  "</td>" +
                "</tr>" 
          )
        }
      }
      top.frames[list_frame].document.write(
              "</td>" +
            "</tr>" +
          "</table>" +
        "</form>"
      );
      top.frames[list_frame].document.close();
    };

    function loadButtons(){
      top.frames[button_frame].document.open();
      top.frames[button_frame].document.write(
        "<form name=buttons>" +
          "<br/>" +
          "<input type='checkbox' name='skip'> " +
            bypass +
          "</input> " +
          "<br/>" +
          "<br/>" +
          "<div align='center'>" +
            "<button onclick='parent.Save();'>" + okCmdLabel + "</button>" +
            " &nbsp;&nbsp;" +
            "<button onclick='parent.Cancel();'>" + cancelCmdLabel + "</button>" +
          "</div>" +
          "<input type='hidden' name='fillins' value=' ' size='-1'>" +
          "<input type='hidden' name='list' value=' ' size='-1'>" +
          "<input type='hidden' name='url' value=' ' size='-1'>" +
        "</form>"
      );
      top.frames[button_frame].document.close();
    }

    function loadFrames(){
      list = DoGetPrefillList();
      BREAK = list[0];
      prefillList = list.split(BREAK);
      loadFillins();
      loadButtons();
    }

    function Save(){
      selname = top.frames[list_frame].document.fSelectFillin;
      var list = top.frames[button_frame].document.buttons.list;
      var url = top.frames[button_frame].document.buttons.url;
      var skip = top.frames[button_frame].document.buttons.skip;
      list.value = prefillList[prefillList.length-2];
      url.value = prefillList[prefillList.length-1];
      var fillins = top.frames[button_frame].document.buttons.fillins;
      fillins.value = "";
      for (i=0; !(i>=selname.length); i++) {
        fillins.value = fillins.value +
          selname.elements[i].options[selname.elements[i].selectedIndex].value + "#*%$" +
          selname.elements[i].options[selname.elements[i].selectedIndex].text + "#*%$";
      }
      var result = "|list|"+list.value+"|fillins|"+fillins.value;
      result += "|url|"+url.value+"|skip|"+skip.checked+"|";
      DoSave(result);
    }

    function Cancel(){
      selname = top.frames[list_frame].document.fSelectFillin;
      var list = top.frames[button_frame].document.buttons.list;
      list.value = prefillList[prefillList.length-2];
      var result = "|list|"+list.value+"|fillins||url||skip|false|";
      DoSave(result);
    }
