    /* for localization */
    var Bundle = srGetStrBundle("chrome://wallet/locale/CookieViewer.properties");
    var siteCookiename = Bundle.GetStringFromName("siteCookiename");
    var cookiesTab = Bundle.GetStringFromName("cookiesTab");
    var permissionsTab = Bundle.GetStringFromName("permissionsTab");
    var cookiesStored = Bundle.GetStringFromName("cookiesStored");
    var name = Bundle.GetStringFromName("name");
    var value = Bundle.GetStringFromName("value");
    var path = Bundle.GetStringFromName("path");
    var secure = Bundle.GetStringFromName("secure");
    var expires = Bundle.GetStringFromName("expires");
    var permissionsStored = Bundle.GetStringFromName("permissionsStored");
    var removeCmdLabel = Bundle.GetStringFromName("removeCmdLabel");
    var okCmdLabel = Bundle.GetStringFromName("okCmdLabel");
    var cancelCmdLabel = Bundle.GetStringFromName("cancelCmdLabel");

    /* for xpconnect */
    var cookieviewer =
      Components.classes
        ["component://netscape/cookieviewer/cookieviewer-world"].createInstance();
    cookieviewer = cookieviewer.QueryInterface(Components.interfaces.nsICookieViewer);

    function DoGetCookieList()
    {
      return cookieviewer.GetCookieValue();
    }

    function DoGetPermissionList()
    {
      return cookieviewer.GetPermissionValue();
    }

    function DoSave(value)
    {
      cookieviewer.SetValue(value, window);
    }
    /* end of xpconnect stuff */

    index_frame = 0;
    title_frame = 1;
    spacer1_frame = 2;
    list_frame = 3;
    spacer2_frame = 4;
    prop_frame = 5;
    spacer3_frame = 6;
    button_frame = 7;

    var cookie_mode;
    var cookieList = [];
    var permissionList =  [];
    deleted_cookies = new Array;
    deleted_permissions = new Array;

    function DeleteItemSelected() {
      if (cookie_mode == 0) {
        DeleteCookieSelected();
      } else if (cookie_mode == 1) {
        DeletePermissionSelected();
      }
    }

    function DeleteCookieSelected() {
      selname = top.frames[list_frame].document.fSelectCookie.selname;
      goneC = top.frames[button_frame].document.buttons.goneC;
      var p;
      var i;
      for (i=selname.options.length; i>0; i--) {
        if (selname.options[i-1].selected) {
          selname.options[i-1].selected = 0;
          goneC.value = goneC.value + selname.options[i-1].value + ",";
          deleted_cookies[selname.options[i-1].value] = 1;
          selname.remove(i-1);
        }
      }
      top.frames[prop_frame].document.open();
      top.frames[prop_frame].document.close();
    }

    function DeletePermissionSelected() {
      selname = top.frames[list_frame].document.fSelectPermission.selname;
      goneP = top.frames[button_frame].document.buttons.goneP;
      var p;
      var i;
      for (i=selname.options.length; i>0; i--) {
        if (selname.options[i-1].selected) {
          selname.options[i-1].selected = 0;
          goneP.value = goneP.value + selname.options[i-1].value + ",";
          deleted_permissions[selname.options[i-1].value] = 1;
          selname.remove(i-1);
        }
      }
    }

    function loadCookies(){
      cookie_mode = 0;
      top.frames[index_frame].document.open();
      top.frames[index_frame].document.write(
        "<body bgcolor='#c0c0c0'>" +
          "<table border='0' width='100%'>" +
            "<tr>" +
              "<td align='center' valign='middle' bgcolor='#ffffff'>" +
                "<font size='2' color='#666666'>" +
                  "<b>" + cookiesTab + "</b>" +
                "</font>" +
              "</td>" +
              "<td align='center' valign='middle' bgcolor='#c0c0c0'>" +
                "<a onclick='top.loadPermissions();' href=''>" +
                  "<font size='2'>" + permissionsTab + "</font>" +
                "</a>" +
              "</td>" +
            "</tr>" +
          "</table>" +
        "</body>"
      );
      top.frames[index_frame].document.close();

      top.frames[title_frame].document.open();
      top.frames[title_frame].document.write
        ("&nbsp;" + cookiesStored);
      top.frames[title_frame].document.close();

      top.frames[prop_frame].document.open();
      top.frames[prop_frame].document.close();

      loadCookiesList();
    }

    function ViewCookieSelected() {
      index = 8*(top.frames[list_frame].document.fSelectCookie.selname.selectedIndex) + 1;
      top.frames[prop_frame].document.open();
      top.frames[prop_frame].document.write(
        "<nobr><b>" + name + "</b> " + cookieList[index+1] + "</nobr><br/>" +
        "<nobr><b>" + value + "</b> " + cookieList[index+2] + "</nobr><br/>" +
        "<nobr><b>" + cookieList[index+3] + ": </b>" + cookieList[index+4] + "</nobr><br/>" +
        "<nobr><b>" + path + "</b> " + cookieList[index+5] + "</nobr><br/>" +
        "<nobr><b>" + secure + "</b> " + cookieList[index+6] + "</nobr><br/>" +
        "<nobr><b>" + expires + "</b> " + cookieList[index+7] + "</nobr><br/>"
      );
      top.frames[prop_frame].document.close();
    }

    function loadCookiesList(){
      top.frames[list_frame].document.open();
      top.frames[list_frame].document.write(
        "<form name='fSelectCookie'>" +
          "<p>" +
            "<b>" + Bundle.GetStringFromName("siteCookiename") + " </b>" +
            "<table border='0'>" +
              "<tr>" +
                "<td width='100%' valign='top'>" +
                  "<center>" +
                    "<p>" +
                      "<select name='selname' size='10' multiple='multiple' onchange='top.ViewCookieSelected();'>"
      );
      for (i=1; !(i>=cookieList.length); i+=8) {
        if (!deleted_cookies[cookieList[i]]) {
          top.frames[list_frame].document.write(
                        "<option value=" + cookieList[i] + ">" +
                          cookieList[i+4] + ":" + cookieList[i+1] +
                        "</option>"
          );
        }
      }
      top.frames[list_frame].document.write(
                      "</select>" +
                    "</p>" +
                  "</center>" +
                "</td>" +
              "</tr>" +
            "</table>" +
          "</p>" +
        "</form>"
      );
      top.frames[list_frame].document.close();
    }

    function loadPermissions(){
      cookie_mode = 1;
      top.frames[index_frame].document.open();
      top.frames[index_frame].document.write(
        "<body bgcolor='#c0c0c0'>" +
          "<table border='0' width='100%'>" +
            "<tr>" +
              "<td align='center' valign='middle' bgcolor='#c0c0c0'>" +
                "<a onclick='top.loadCookies();' href=''>" +
                  "<font size='2'>" + cookiesTab + "</font>" +
                "</a>" +
              "</td>" +
              "<td align='center' valign='middle' bgcolor='#ffffff'>" +
                "<font size='2' color='#666666'>" +
                  "<b>" + permissionsTab + "</b>" +
                "</font>" +
              "</td>" +
              "<td>&nbsp;&nbsp;&nbsp;</td>" +
            "</tr>" +
          "</table>" +
        "</body>"
      );
      top.frames[index_frame].document.close();

      top.frames[title_frame].document.open();
      top.frames[title_frame].document.write
        ("&nbsp;" + permissionsStored + "");
      top.frames[title_frame].document.close();

      top.frames[prop_frame].document.open();
      top.frames[prop_frame].document.close();

      loadPermissionsList();
    }

    function loadPermissionsList(){
      top.frames[list_frame].document.open();
      top.frames[list_frame].document.write(
        "<form name='fSelectPermission'>" +
          "<p>" +
            "<table border='0'>" +
              "<tr>" +
                "<td width='100%' valign='top'>" +
                  "<center>" +
                    "<p>" +
                      "<select name='selname' size='10' multiple='multiple'> "
      );
      for (i=1; !(i>=permissionList.length); i+=2) {
        if (!deleted_permissions[permissionList[i]]) {
          top.frames[list_frame].document.write(
                        "<option value=" + permissionList[i] + ">" +
                          permissionList[i+1] +
                        "</option>"
          );
        }
      }
      top.frames[list_frame].document.write(
                      "</select>" +
                    "</p>" +
                  "</center>" +
                "</td>" +
              "</tr>" +
            "</table>" +
          "</p>" +
        "</form>"
      );
      top.frames[list_frame].document.close();
    }

    function loadButtons(){
      top.frames[button_frame].document.open();
      top.frames[button_frame].document.write(
        "<form name='buttons'>" +
          "<br/>" +
          "&nbsp;" +
          "<button onclick='top.DeleteItemSelected();'>" + removeCmdLabel + "</button>" +
          "<div align='right'>" +
            "<button onclick='parent.Save();'>" + okCmdLabel + "</button>" +
            " &nbsp;&nbsp;" +
            "<button onclick='parent.Cancel();'>" + cancelCmdLabel + "</button>" +
          "</div>" +
          "<input type='hidden' name='goneC' value='' size='-1'/>" +
          "<input type='hidden' name='goneP' value='' size='-1'/>" +
          "<input type='hidden' name='cookieList' value='' size='-1'/>" +
          "<input type='hidden' name='permissionList' value='' size='-1'/>" +
        "</form>"
      );
      top.frames[button_frame].document.close();
    }

    function loadFrames(){

      /*
       * The cookieList is a sequence of items separated by the BREAK character.  These
       * items are:
       *   empty
       *   number for first cookie
       *   name for first cookie
       *   value for first cookie
       *   domain indicator ("Domain" or "Host") for first cookie
       *   domain or host name for first cookie
       *   path for first cookie
       *   secure indicator ("Yes" or "No") for first cookie
       *   expiration for first cookie
       * with the eight items above repeated for each successive cookie
       */
      list = DoGetCookieList();
      BREAK = list[0];
      cookieList = list.split(BREAK);

      /*
       * The permissionList is a sequence of items separated by the BREAK character.  These
       * items are:
       *   empty
       *   number for first permission
       *   +/- hostname for first permission
       * with the above two items repeated for each successive permission
       */
      list = DoGetPermissionList();
      BREAK = list[0];
      permissionList = list.split(BREAK);
      loadCookies();
      loadButtons();
    }

    function Save(){
      var goneC = top.frames[button_frame].document.buttons.goneC;
      var goneP = top.frames[button_frame].document.buttons.goneP;
      var result = "|goneC|"+goneC.value+"|goneP|"+goneP.value+"|";
      DoSave(result);
    }

    function Cancel(){
      var result = "|goneC||goneP||";
      DoSave(result);
    }

