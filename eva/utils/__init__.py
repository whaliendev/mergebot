
from typing import List


def lang_exts(lang: str) -> List[str]:
    """
    Returns a list of file extensions associated with a given programming language.

    Parameters:
    lang (str): The programming language. Currently supports 'c' and 'kt'.

    Returns:
    List[str]: A list of file extensions. For 'c', it returns extensions for C and C++ files.
    For 'kt', it returns extensions for Kotlin files. For other languages, it returns a list containing the language name.

    Examples:
    >>> lang_exts('c')
    ['c', 'h', 'cpp', 'hpp', 'cc', 'hh', 'cxx', 'hxx', 'C', 'cx', 'c++', 'cp']

    >>> lang_exts('kt')
    ['kt', 'kts']

    >>> lang_exts('py')
    ['py']
    """
    if lang == 'c':
        return ["c", "h", "cpp", "hpp", 'cc', 'hh', 'cxx', 'hxx', "C", "cx", "c++", "cp"]
    elif lang == 'kt':
        return ["kt", "kts"]
    else:
        return [lang]
