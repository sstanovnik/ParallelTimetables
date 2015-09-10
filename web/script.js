"use strict";

var TimetableApp = angular.module('TimetableApp', ['TimetableControllers']);

var TimetableControllers = angular.module('TimetableControllers', []);
TimetableControllers.controller('MainCtrl', ['$scope', function($scope) {
    $scope.refreshTooltips = function() {
        window.setTimeout(function() {
            $('[data-toggle="tooltip"]').tooltip();
        }, 200);
    };

    $scope.listToString = function(list) {
        var result = "";
        var first = true;
        list.forEach(function(el) {
            if (!first) {
                result += ", ";
            }
            first = false;
            result += el;
        });
        return result;
    };

    var addOrAppend = function(object, key, value) {
        if (object[key] === undefined) {
            object[key] = [];
        }

        object[key].push(value);
    };

    $.getJSON("../out/timetable.json", function(timetableJson) {
        $scope.students = {};
        $scope.professors = {};
        $scope.classrooms = {};
        $scope.subjects = {};

        // do post-processing
        timetableJson.timetable_entries.forEach(function(timetableEntry) {
            addOrAppend($scope.classrooms, timetableEntry.classroom, timetableEntry);

            timetableEntry.professors.forEach(function(professor) {
                addOrAppend($scope.professors, professor, timetableEntry);
            });

            timetableEntry.students.forEach(function(student) {
                addOrAppend($scope.students, student, timetableEntry);
            });

            addOrAppend($scope.subjects, timetableEntry.subject, timetableEntry);
        });

        $scope.$apply(); // seems to be necessary
    });
}]);
