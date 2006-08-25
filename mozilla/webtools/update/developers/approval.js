// Check all the checkboxes for the specified item
function checkAll(id) {
    document.getElementById('installation_'+id).checked = true;
    document.getElementById('uninstallation_'+id).checked = true;
    document.getElementById('appworks_'+id).checked = true;
    document.getElementById('cleanprofile_'+id).checked = true;
    if(document.getElementById('newchrome_'+id))
        document.getElementById('newchrome_'+id).checked = true;
    if(document.getElementById('worksasdescribed_'+id))
        document.getElementById('worksasdescribed_'+id).checked = true;
    if(document.getElementById('visualerrors_'+id))
        document.getElementById('visualerrors_'+id).checked = true;
    if(document.getElementById('allelementsthemed_'+id))
        document.getElementById('allelementsthemed_'+id).checked = true;

    document.getElementById('checkall_'+id).style.display = 'none';
}

// Show the form for the specified item
function showForm(id) {
    document.getElementById('form_'+id).style.display = '';
    document.getElementById('showform_'+id).style.display = 'none';
}
