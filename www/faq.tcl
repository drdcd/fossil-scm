#!/usr/bin/tclsh
#
# Run this to generate the FAQ
#
set cnt 1
proc faq {question answer} {
  set ::faq($::cnt) [list [string trim $question] [string trim $answer]]
  incr ::cnt
}

faq {
  What GUIs are available for fossil?
} {
  The fossil executable comes with a web-based GUI built in.  Just run:

  <blockquote>
  <b>fossil ui</b> <i>REPOSITORY-FILENAME</i>
  </blockquote>

  And your default web browser should pop up and automatically point to
  the fossil interface.  (Hint:  You can omit the <i>REPOSITORY-FILENAME</i>
  if you are within an open check-out.)
}

faq {
  What is the difference between a "branch" and a "fork"?
} {
  This is a big question - too big to answer in a FAQ.  Please
  read the <a href="branching.wiki">Branching, Forking, Merging,
  and Tagging</a> document.
}


faq {
  How do I create a new branch in fossil?
} {
  There are lots of ways:

  When you are checking in a new change using the <b>commit</b>
  command, you can add the option  "--branch <i>BRANCH-NAME</i>" to
  make the change be the founding check-in for a new branch.  You can
  also add the "--bgcolor <i>COLOR</i>" option to give the branch a
  specific background color on timelines.

  If you want to create a new branch whose founding check-in is the
  same as an existing check-in, use this command:

  <blockquote>
  <b>fossil branch new</b> <i>BRANCH-NAME BASIS</i>
  </blockquote>

  The <i>BRANCH-NAME</i> argument is the name of the new branch and the
  <i>BASIS</i> argument is the name of the check-in that the branch splits
  off from.

  If you already have a fork in your check-in tree and you want to convert
  that fork to a branch, you can do this from the web interface.
  First locate the check-in that you want to be
  the founding check-in of your branch on the timeline and click on its
  link so that you are on the <b>ci</b> page.  Then find the "<b>edit</b>"
  link (near the "Commands:" label) and click on that.  On the 
  "Edit Check-in" page, check the box beside "Branching:" and fill in 
  the name of your new branch to the right and press the "Apply Changes"
  button.
}

faq {
  How do I create a private branch that won't get pushed back to the
  main repository.
} {
  Use the <b>--private</b> command-line option on the 
  <b>commit</b> command.  The result will be a check-in which exists on
  your local repository only and is never pushed to other repositories.  
  All descendents of a private check-in are also private.
  
  Unless you specify something different using the <b>--branch</b> and/or
  <b>--bgcolor</b> options, the new private check-in will be put on a branch
  named "private" with an orange background color.
  
  You can merge from the trunk into your private branch in order to keep
  your private branch in sync with the latest changes on the trunk.  Once
  you have everything in your private branch the way you want it, you can
  then merge your private branch back into the trunk and push.  Only the
  final merge operation will appear in other repositories.  It will seem
  as if all the changes that occurred on your private branch occurred in
  a single check-in.
  Of course, you can also keep your branch private forever simply
  by not merging the changes in the private branch back into the trunk.
}

faq {
  How can I delete inappropriate content from my fossil repository?
} {
  See the article on [./shunning.wiki | "shunning"] for details.
}



#############################################################################
# Code to actually generate the FAQ
#
puts "<h1 align=\"center\">Frequently Asked Questions</h1>\n"
puts "<p>Note: See also <a href=\"qandc.wiki\">Questions and Criticisms</a>.\n"

puts {<ol>}
for {set i 1} {$i<$cnt} {incr i} {
  puts "<li><a href=\"#q$i\">[lindex $faq($i) 0]</a></li>"
}
puts {</ol>}
puts {<hr>}

for {set i 1} {$i<$cnt} {incr i} {
  puts "<a name=\"q$i\"></a>"
  puts "<p><b>($i) [lindex $faq($i) 0]</b></p>\n"
  set body [lindex $faq($i) 1]
  regsub -all "\n *" [string trim $body] "\n" body
  puts "<blockquote>$body</blockquote></li>\n"
}
puts {</ol>}
