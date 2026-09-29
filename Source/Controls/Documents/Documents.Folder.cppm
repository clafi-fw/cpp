export module ClaFi.Documents.Folder;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.DirWatch;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // What one kind of document is called and how its files are named. See Documents
    export struct DocumentKind
    {
        std::wstring_view extension;    // what makes a file one of these, dot included
        std::wstring_view noun;         // one of them, as a sentence names it - "script"
        std::wstring_view nounPlural;   // several of them - "scripts"
        std::wstring_view newStem;      // the name a new one is made under - "New Script"
        std::wstring_view attrName;     // the tab config attribute a page keeps its document under
    };

    export using FilePaths = std::vector<std::filesystem::path>;

    // A folder of one kind of document, watched while the application is up. See Documents
    export class DocumentsFolder
    {
    public:
        // Told when the folder's files have changed on the disk.
        class IListener
        {
            friend DocumentsFolder;
        public:
            virtual ~IListener() = default;
        protected:
            virtual void documentsFolderChanged() = 0;
        };
    public:
        DocumentsFolder(const std::filesystem::path& directory, const DocumentKind&);
        virtual ~DocumentsFolder() = default;
        DocumentsFolder(const DocumentsFolder&) = delete;
        DocumentsFolder& operator=(const DocumentsFolder&) = delete;
    public:
        [[nodiscard]] std::set<IListener*>& listeners() { return m_listeners; }
        // Empty where the platform names no folder, and then nothing is ever written.
        [[nodiscard]] const std::filesystem::path& directory() const { return m_directory; }
        [[nodiscard]] const DocumentKind& kind() const { return m_kind; }
        // The files in the folder, sorted by name - read from the disk on the first ask after the
        // folder has changed.
        [[nodiscard]] const FilePaths& files();
        [[nodiscard]] std::filesystem::path fileOf(std::wstring_view fileName) const;
        [[nodiscard]] std::wstring fileNameOf(std::wstring_view stem) const;
        // Makes the folder where there is none yet, and answers whether it had to.
        bool needDirectory();
        // Makes an empty file under the kind's new stem, numbered past the names taken. Answers
        // its file name, and nothing where nothing was written.
        [[nodiscard]] std::wstring createFile();
        // Renames a file to the stem an editor was left with, or refuses the name and says why.
        // Answers the new name, and nothing where the file did not move. See Documents#rename
        [[nodiscard]] std::wstring renameFile(const std::filesystem::path&, AcceptEditEvent&) const;
        // Refuses a name that is no name at all. Whether the name is free is the caller's.
        virtual void checkNameShape(AcceptEditEvent&, std::wstring_view stem) const;
        // Reads a file's document into the node, whole, and answers whether it could. A file that
        // cannot be read leaves the empty document there. See Documents#read
        [[nodiscard]] virtual bool readDocument(const std::filesystem::path&,
            Dom::DomNodeBase& into) const = 0;
        // Writes the node's document to a file, and answers whether it got there.
        [[nodiscard]] virtual bool writeDocument(const Dom::DomNodeBase&,
            const std::filesystem::path&) const = 0;
        // Whether a file holds work, which is what is asked about before the file is deleted.
        [[nodiscard]] virtual bool isEdited(const std::filesystem::path&) const = 0;
        // A document's mark, in the inks of where it stands: a crumb, a tab, a tile.
        virtual void paintIcon(std::wstring_view fileName, PaintIconEvent&) = 0;
    private:
        [[nodiscard]] std::filesystem::path newFile(int counter) const;
        // Puts the watch over a directory that was made after the watch was set up.
        void watchDirectory();
        void update();
        void notifyListeners();
    private:
        std::filesystem::path m_directory;
        DocumentKind m_kind;
        DirWatch m_dirWatcher{ m_directory };
        ScopedEventConnection m_dirWatchConnection{};
        bool m_loaded{ false };
        FilePaths m_files{};
        std::set<IListener*> m_listeners{};
    };
}
