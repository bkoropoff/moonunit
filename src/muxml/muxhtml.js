var slideDuration = 200
var menuDuration = 100

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

function buildLibMenus()
{
    var d = document;
    var libs = $(".library");

    $(libs).each(function(i, library) {
        var menu = d.createElement("div");
        menu.className = "menu";
        
        $(libs).each(function(j, other) {
            if (other != library) {
                var name = other.getAttribute("library-name");
                var entry = d.createElement("div");
                entry.className = "entry";
                entry.appendChild(d.createTextNode(name));
                menu.appendChild(entry);
            }
        });
        
        $(library).children(".header").append(menu);
        $(menu).hide();
    });
}

function toggleMenu() {
    $(this).children(".title").toggleClass("opened");
    $(this).children(".menu").toggle();
}

function closeMenu() {
    $(this).children(".title").removeClass("opened");
    $(this).children(".menu").hide();
}

function switchLibrary() {
    var menu = $(this).parent()[0];
    var title = $(menu).siblings(".title")[0];
    var library = $(this).parents(".library")[0];

    $(menu).hide();
    $(title).toggleClass("opened");
    $(library).hide();
    $(".library[library-name='" + this.firstChild.nodeValue + "']").show();
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

    $(".library > .header").each(function(){
        var header = this;
        $(this).children(".title").click(function() {
            toggleMenu.call(header);
        })
    });

    $(".library > .header > .menu > .entry").click(switchLibrary);

    $(document).click(function(e){
        $(".library > .header > .menu").each(function() {
            title = $(this).siblings(".title")[0];
            if (!$(e.target).closest(this).length &&
                !$(e.target).closest(title).length) {
                closeMenu.call(this.parentNode);
            }
        });
    });
}

function initialize() {
    $(".test > .content").slideUp();

    $(".test:has(.content)").each(function(i, test) {
        var content = $(test).find(".content")[0];
        $(test).find(".header").click(function() {
            $(content).slideToggle(slideDuration);
        });
    });

    buildLibMenus();
    $(".library").hide();
    var libZero = $(".library")[0];
    $(libZero).show();

    connectUi();
}

$(document).ready(initialize);
