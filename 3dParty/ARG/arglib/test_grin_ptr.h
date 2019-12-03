namespace arg_test
{
	class outer
	{
	public:
		outer();
		
		int get_current_instances() const;
		int get_copy_count() const;
		int get_delete_count() const;
		
	private:
		class inner;
		arg::grin_ptr<inner> body;
	};
}

