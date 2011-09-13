var slideDuration = 200

function filterTests(name, pass, fail, skip) {
    function matchesFilter(i, test) {
        var nameLower = test.getAttribute("test-name").toLowerCase();
        var status = test.getAttribute("test-status");
        return ((name == null || nameLower.indexOf(name.toLowerCase()) != -1) &&
                ((pass && (status == "pass" || status == "xfail")) ||
                 (fail && (status == "fail" || status == "xpass")) ||
                 (skip && status == "skip")))
    }

    $(".suite").each(function(i, suite) {
        if ($(suite).find(".test").filter(matchesFilter).length == 0) {
            $(suite).hide();
        }
        else {
            $(suite).show();
            $(suite).find(".test").each(function(j, test) {
                if (matchesFilter(j, test)) {
                    $(test).show();
                }
                else {
                    $(test).hide();
                }
            });
        }
    });
}

function connectUi() {
    var filterReset = $("#filter-reset-button")[0];
    var filterName = $("#filter-name")[0];
    var filterPass = $("#filter-pass")[0];
    var filterFail = $("#filter-fail")[0];
    var filterSkip = $("#filter-skip")[0];

    function applyFilter() {
        filterTests(
            filterName.value,
            filterPass.checked,
            filterFail.checked,
            filterSkip.checked);
    }

    $([filterName, filterPass, filterFail, filterSkip]).change(applyFilter);

    $(filterReset).click(function() {
        filterName.value = "";
        filterPass.checked = true;
        filterFail.checked = true;
        filterSkip.checked = true;
        applyFilter();
    })
}

function initialize() {
    $(".test > .content").slideUp();

    $(".test:has(.content)").each(function(i, test) {
        var content = $(test).find(".content")[0];
        $(test).find(".header").click(function() {
            $(content).slideToggle(slideDuration);
        });
    });

    connectUi();
}

$(document).ready(initialize);
